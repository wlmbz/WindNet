//=============================================================================
/**
*  file: serverClient.h
*
*  Brief:��Ҫ�Ƿ�������ʹ�õ������װ����IOCP��ʽʵ�ֵ�����ӿ�
*		����һ���ͻ��˵��������󵽴�󣬷���������һ��ServerClient��֮����
*		���ฺ��;���ͻ���ͨѶ
*
*  Authtor:wangqiao
*	
*	Datae:2005-4-15
*
*	Modify:2007-4-13,�����˴���ͽṹ���Ż���Ч��
*/
//=============================================================================

#include "serverClient.h"
#include <mmsystem.h>
#include <assert.h>
#include "BaseMessage.h"
#include "Servers.h"
#include "base/Crc32Static.h"


using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServerClient::CServerClient(CServer*	pServers,CDataBlockAllocator* pDBAllocator)
:m_pServers(pServers)
,m_pDBAllocator(pDBAllocator)
{
    m_nUseCount=1;
    m_bServerType = 0;
    Init();
}


void CServerClient::Init()
{
    assert(m_pServers);
    m_bLost = false;
    m_bShutDown = false;
    m_bQuit = false;
    m_bUsing= false;
    m_bTransfersStatus = false;
    m_bAccept = true;
    m_bSendZeroByteData = false;
    m_nReadDataSize = 0;
    m_ReadSequenceNumber = 1;
    m_CurrentReadSequenceNumber = 1;
    m_SendMessages.clear();
    m_ReadDataBlocks.clear();


    //ͳ�ƽ���
    m_dwRcvStartTime = timeGetTime();		//����ͳ�ƿ�ʼʱ��
    m_lCurTotalRcvSize = 0;					//���ܰ����ܴ�С
    m_lCurRcvSizePerSecond = 0;				//��ǰÿ����ܵ����ݴ�С
    m_lMaxRcvSizePerSecond = 4096;			//���ÿ����ܴ�С

    //ͳ�Ʒ���
    m_dwSendStartTime = timeGetTime();		//����ͳ�ƿ�ʼʱ��
    m_lCurTotalSendSize = 0;				//��ǰ�ܷ��ʹ�С
    m_lCurSendSizePerSecond = 0;			//��ǰÿ�뷢�ʹ�С
    m_lMaxSendSizePerSecond = 8192;			//ÿ������ʹ�С

    m_bChecked = false;
    m_lMaxMsgLen  = 0x7FFFFFFF;				//�����Ϣ����
    m_lPendingWrBufNum = 0;					//��ǰ�����ͻ��������ܴ�С
    m_lMaxPendingWrBufNum = 8192;			//�������ͻ��������ܴ�С
    m_lMaxPendingRdBufNum = 8192;			//����Ľ��ܻ��������ܴ�С
    m_lMaxBlockSendMsnNum = 1024;			//�������δ������Ϣ����

    m_lSendCounter = 0;
    m_lRecvCounter = 0;


    m_lLastMsgTotalSize = 0;
    //�ϴ���Ϣ�ĳ���
    m_lLastMsgLen = 0;
    //�ϴ���Ϣ������
    m_lLastMsgType = 0;
    //�ϴ���Ϣ���ݿ�Ĵ���ǰ����
    m_lLastMsgPreDBNum = 0;
    //�ϴ���Ϣ���ݿ�Ĵ�������
    m_lLastMsgPosDBNum = 0;
    m_lLastMsgRemainSize = 0;
    //�ϴ���Ϣ�ƶ��ڴ��λ��
    m_lLastMsgMemMoveDBPos = 0;
    //�ϴ��ƶ������
    m_lLastMsgMemMoveDBSize = 0;

}

void CServerClient::Release()
{
    itMsg it = m_SendMessages.begin();
    for(;it != m_SendMessages.end();it++)
    {
        CBaseMessage* pMsg = (*it);
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
    m_SendMessages.clear();

    itDB itDB = m_ReadDataBlocks.begin();
    for(;itDB != m_ReadDataBlocks.end() ;itDB++)
    {
        m_pDBAllocator->FreeDB((*itDB));
    }
    m_ReadDataBlocks.clear();

    itDBMap itDBBuff = m_ReadBuffer.begin();
    for(;itDBBuff != m_ReadBuffer.end();itDBBuff++)
    {
        m_pDBAllocator->FreeDB((*itDBBuff).second);
    }
    m_ReadBuffer.clear();
}

CServerClient::~CServerClient()
{
    Release();
}

//������Ӧ����
void CServerClient::SetParam(bool bChecked,long lMaxMsgLen,long lMaxBlockSendMsgNum,
                             long lMaxPendingWrBufNum,long lMaxPendingRdBufNum,
                             long lMaxSendSizePerSecond,long lMaxRcvSizePerSecond,
                             long lFlag)
{
    m_bChecked = bChecked;
    m_lMaxMsgLen  = lMaxMsgLen;	
    m_lMaxBlockSendMsnNum = lMaxBlockSendMsgNum;

    m_lMaxPendingWrBufNum = lMaxPendingWrBufNum;
    m_lMaxPendingRdBufNum = lMaxPendingRdBufNum;

    m_lMaxSendSizePerSecond = lMaxSendSizePerSecond;
    m_lMaxRcvSizePerSecond = lMaxRcvSizePerSecond;

    lFlag = lFlag;
}

bool CServerClient::StartReadData()
{
    int nRecBuf = 0;
    while(nRecBuf<m_lMaxPendingRdBufNum)
    {
        PER_IO_OPERATION_DATA* pPerIOData = m_pServers->AllocIoOper();
        if( NULL==pPerIOData)	return false;
        CDataBlock *pDB = m_pDBAllocator->AllocDB(7);

        pPerIOData->hSocket = m_hSocket;
        //����ɶ˿��ϻ������
        if(ReadFromCompletionPort(pPerIOData,pDB)==false)
        {
            m_pServers->FreeIoOper(pPerIOData);
            return false;
        }
        nRecBuf += pDB->GetMaxSize();
    }
    return true;
}
//**********************************
//�����ɶ˿����յ�������
//**********************************
bool CServerClient::ReadFromCompletionPort(PER_IO_OPERATION_DATA* pIOData,CDataBlock *pDB)
{	
    if(NULL == pIOData ) return false;
    if(NULL == pDB)
        pDB = m_pDBAllocator->AllocDB(8);
    if(NULL == pDB)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�ں���CServerClient::ReadFromCompletionPort(...)��,�����ڴ�ʧ�ܣ�");
        return false;
    }

    memset( &pIOData->OverLapped,0,sizeof(OVERLAPPED) );
    if(pIOData->m_lDataBufNum < 1)
        pIOData->ResetDataBufSize(1);
    pIOData->pParam = pDB;
    pIOData->pDataBuf[0].len = pDB->GetMaxSize();
    pIOData->pDataBuf[0].buf = (char*)pDB->Base();
    pIOData->OperationType = SOT_Receive;
    pIOData->m_nSequenceNumber = m_ReadSequenceNumber;
    ulong dwFlag = 0;
    ulong dwReceivByte;
    int nRet = WSARecv(m_hSocket,pIOData->pDataBuf,1,&dwReceivByte,&dwFlag,&(pIOData->OverLapped),NULL);
    if(nRet == SOCKET_ERROR)
    {
        int nError = WSAGetLastError();
        if (nError != WSA_IO_PENDING)
        {
            m_pDBAllocator->FreeDB(pDB);
            //PutErrorString(NET_MODULE,"%-15s �ں���CServerClient::ReadFromCompletionPort(...)��,WSARecv()����ʧ�ܣ�(ErrorID:%d)",__FUNCTION__,nError);
            return false;
        }
    }
    IncrReadSequenceNumber();
    return true;
}



// [TCP]������������ɶ˿�
int CServerClient::Send(const void* lpBuf, int nBufLen, int nFlags)
{
    if( IsShutDown() )	return true;

    if(m_SendMessages.size() == 0 ||//�Ƿ�������
        GetPendingWrBufNum() > m_lMaxPendingWrBufNum)//�����͵����ݴ�СС���޶�ֵ
    {
        return true;
    }
    //���ƽ���������ݴ�С�����޶�ֵ,����0��С������
    if(GetCurSendSizePerSecond() > m_lMaxSendSizePerSecond)
    {
        SendZeroByteData();
        return true;
    }

    itMsg it = m_SendMessages.begin();
    for(;it != m_SendMessages.end();)
    {
        //����һ����Ϣ
        if(!Send((*it),nFlags))
            return false;
        it = m_SendMessages.erase(it);
        if(GetPendingWrBufNum() > m_lMaxPendingWrBufNum ||
            GetCurSendSizePerSecond() > m_lMaxSendSizePerSecond )
            break;
    }
    return true;
}


//��������������������������ʱ,����0�ֽ�����
//��������һ������������⡢��������
void CServerClient::SendZeroByteData()
{
    if(m_bSendZeroByteData)	return;

    PER_IO_OPERATION_DATA* pPerIOData = m_pServers->AllocIoOper();
    //�����䲻���ڴ�ʱ
    if(NULL == pPerIOData) return;
    memset( &pPerIOData->OverLapped,0,sizeof(OVERLAPPED) );
    pPerIOData->OperationType = SOT_SendZeroByte;
    bool bSuccess = PostQueuedCompletionStatus(m_pServers->m_hCompletionPort,0,(ulong)GetIndexID(),(OVERLAPPED*)pPerIOData) ? true : false;
    if( !bSuccess && WSAGetLastError() != ERROR_IO_PENDING )
    {
        //PutErrorString(NET_MODULE,"%-15sCServerClient::SendZeroByteData().PostQueuedCompletionStatus .erros(errid:%d).",__FUNCTION__,WSAGetLastError());
        return;
    }
    m_bSendZeroByteData = true;
    return;
}

void CServerClient::OnSendZeroByteData()
{
    m_bSendZeroByteData = false;
}


int CServerClient::Send(CBaseMessage *pMsg,int nFlags)
{
    if(NULL == pMsg)	return false;

    //��ӷ�����Ϣͳ��
    //m_pServers->AddSendMsgStat(pMsg->GetType(),pMsg->GetSize());

    PER_IO_OPERATION_DATA* pPerIOData = m_pServers->AllocIoOper();
    //�����䲻���ڴ�ʱ
    if(NULL == pPerIOData) return false;
    long lTotalSendSize = 0, lTotalMsgSize = pMsg->GetTotalSize();
    if (m_pServers->IsEncryptType(GetFlag()))
    {
        pMsg->Encrypt(m_kn);
    }

    memset( &pPerIOData->OverLapped,0,sizeof(OVERLAPPED) );		
    pPerIOData->OperationType = SOT_Send;
    pPerIOData->pParam = pMsg;
    pPerIOData->hSocket = m_hSocket;

    vector<CDataBlock*>& DataBlocks = pMsg->GetMsgData();
    long lBlockSize = (long)DataBlocks.size();
    if(pPerIOData->m_lDataBufNum < lBlockSize)
        pPerIOData->ResetDataBufSize(lBlockSize);
    int i = 0;
    for(;i<lBlockSize;i++)
    {
        CDataBlock* pDB = DataBlocks[i];
        pPerIOData->pDataBuf[i].buf=(char*)pDB->Base();
        pPerIOData->pDataBuf[i].len = pDB->GetCurSize();
        lTotalSendSize += pDB->GetCurSize();
    }

    //����
    if (lTotalSendSize != lTotalMsgSize)
    {
        free(pPerIOData);		
        //PutErrorString(NET_MODULE,"%-15s Msg Length Error(NetFlag:%d,IndexID:%d,BlockSize:%d,MsgType:%d,MsgSize:%d,SendSize:%d)",
        //    __FUNCTION__,GetIndexID(),lBlockSize,pMsg->GetType(),pMsg->GetTotalSize(),lTotalSendSize);
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
        return true;
    }

    ulong dwSentNum = 0;
    int ret = WSASend(m_hSocket, pPerIOData->pDataBuf, lBlockSize, &dwSentNum, nFlags,
        (OVERLAPPED*)pPerIOData, NULL);

    if (ret == SOCKET_ERROR)
    {
        int nError = WSAGetLastError();
        if ( nError != WSA_IO_PENDING)
        {
            free(pPerIOData);
            //PutErrorString(NET_MODULE,"%-15s ��ͻ��˷�����Ϣ����(errorID:%d)",__FUNCTION__,nError);
            return false;
        }
    }

    AddSendSize(lTotalSendSize);
    IncPendingWrBufNum(lTotalSendSize);
    return true;
}

bool CServerClient::AddSendMsg(CBaseMessage* pMsg)
{
    char strInfo[256]="";
    if(NULL == pMsg)	return false;
    if( IsShutDown() )
    {
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
        return false;
    }
    //�����������Ϣ�������ڹ涨��������ǿ���ԶϿ�������
    if(m_SendMessages.size() > m_lMaxBlockSendMsnNum)
    {
        ShutDown();
        if(pMsg->RemoveRefCount() == 0)
        {
            CBaseMessage::FreeMsg(pMsg);
        }
        //PutErrorString(NET_MODULE,"%-15s the blocked send messge count(num:%d) greater the max count(num:%d)",
        //    __FUNCTION__,m_SendMessages.size(),m_lMaxBlockSendMsnNum);

        return false;
    }

    //OnTransferChange();
    //ulong dwCurTime = timeGetTime();
    //long lCurMsgPri = pMsg->GetPriorityValue(dwCurTime);
    ////����Ϣ�����Ƿ���Զ���
    //bool bDiscard = CBaseMessage::GetIsDiscard(pMsg->GetType());
    ////�������ȼ��ж�,ȷ�������λ��
    //itMsg it  = m_SendMessages.begin();
    //for(;it != m_SendMessages.end();)
    //{
    //	CBaseMessage* pTemptMsg = (*it);
    //	//�����Ϣ����һ�����鿴�Ƿ���Զ����Ѵ��ڵ���Ϣ
    //	if(pMsg->GetType() == pTemptMsg->GetType())
    //	{
    //		//����������һ���������Ѿ����ڵ���Ϣ
    //		if(bDiscard && pMsg->IsDiscardFlagEqual(pTemptMsg) )
    //		{
    //			//_snprintf(strInfo,256,"Discard a messge(type:%d)",pTemptMsg->GetType());
    //			//PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,strInfo);
    //			it = m_SendMessages.erase(it);
    //			CBaseMessage::FreeMsg(pTemptMsg);
    //			continue;
    //		}
    //	}
    //	//���Ͳ�һ�����ж����ȼ�
    //	else
    //	{
    //		if( lCurMsgPri > pTemptMsg->GetPriorityValue(dwCurTime))
    //		{
    //			_snprintf(strInfo,256,"the messge(type:%d) prio greater the msg(type:%d)",
    //						pMsg->GetType(),pTemptMsg->GetType());
    //			PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,strInfo);
    //			break;
    //		}
    //	}
    //	it++;
    //}
    //
    //m_SendMessages.insert(it,pMsg);
    m_SendMessages.push_back(pMsg);
    //����ѷ��͵�����С�����ô�С����������
    if( !Send(NULL,0,0) )
        return false;
    return true;
}

bool CServerClient::AddReceiveData(long lSequeNumber,CDataBlock *pDB)
{
    if(NULL == pDB)	return false;

    itDBMap it = m_ReadBuffer.find(m_CurrentReadSequenceNumber);
    if(it != m_ReadBuffer.end())
    {
        //Log4c::Warn(NET_MODULE,"The key has already exist in the read-buffs");
    }

    if(lSequeNumber == m_CurrentReadSequenceNumber)
    {
        PushReadDataBack(pDB);
        IncrCurReadSequenceNumber();
    }
    else
    {
        m_ReadBuffer[lSequeNumber] = pDB;
    }

    while(pDB = GetNextReadDB())
    {
        PushReadDataBack(pDB);
        IncrCurReadSequenceNumber();
    }
    return true;
}

CDataBlock* CServerClient::GetNextReadDB()
{
    CDataBlock *pDB = NULL;
    itDBMap it = m_ReadBuffer.find(m_CurrentReadSequenceNumber);
    if(it != m_ReadBuffer.end())
    {
        pDB = (*it).second;
        m_ReadBuffer.erase(it);
    }
    return pDB;
}

//������ͷ��ʼ���һ��longֵ
long CServerClient::GetCurMsgLen()
{
    char strInfo[1024]="";
    //��һ�����ֵ
    long len = 0x7FFFFFFF;
    if(m_nReadDataSize < 4)	return len;

    BYTE* pLen = (BYTE*)&len;
    long lPos = 0;
    long size = sizeof(long);	
    itDB it = m_ReadDataBlocks.begin();
    for(;it != m_ReadDataBlocks.end() && size > 0;it++)
    {
        CDataBlock *pDB = (*it);
        if(pDB)
        {
            long minSize = min(size,pDB->GetCurSize());
            memcpy(pLen+lPos,pDB->Base(),minSize);
            if(len<=0)
            {
                //PutErrorString(NET_MODULE,"%-15s Get MsgLen Error1!(flag:%d,Index:%d)len:%d,ReadDataBlocksSize:%d,minSize:%d.(m_lLastMsgTotalSize:%d,\
                //             m_lLastMsgLen:%d,m_lLastMsgType:%d,m_lLastMsgPreDBNum:%d,m_lLastMsgPosDBNum:%d,m_lLastMsgMemMoveDBPos:%d,\
                //             m_lLastMsgMemMoveDBSize:%d,m_lLastMsgRemainSize:%d)",
                //             __FUNCTION__,len,m_ReadDataBlocks.size(),minSize,m_lLastMsgTotalSize,m_lLastMsgLen,m_lLastMsgType,m_lLastMsgPreDBNum,
                //             m_lLastMsgPosDBNum,m_lLastMsgMemMoveDBPos,m_lLastMsgMemMoveDBSize,m_lLastMsgRemainSize);
            }
            lPos+=minSize;
            size -= minSize;
            //�˳�
            if(size <= 0 )
                break;
        }
    }

    if(m_nReadDataSize >= 4)
    {
        if(m_bChecked && len > m_lMaxMsgLen)
        {
           // PutErrorString(NET_MODULE,"Get MsgLen Error2!(flag:%d,Index:%d)len:%d,MaxMsgLen:%d(m_lLastMsgTotalSize:%d,m_lLastMsgLen:%d,\
           //              m_lLastMsgType:%d,m_lLastMsgPreDBNum:%d,m_lLastMsgPosDBNum:%d,m_lLastMsgMemMoveDBPos:%d,m_lLastMsgMemMoveDBSize:%d,\
           //              m_lLastMsgRemainSize:%d)",
           //              __FUNCTION__,GetFlag(),GetIndexID(),len,m_lMaxMsgLen,m_lLastMsgTotalSize,m_lLastMsgLen,m_lLastMsgType, 
           //              m_lLastMsgPreDBNum,m_lLastMsgPosDBNum,m_lLastMsgMemMoveDBPos,m_lLastMsgMemMoveDBSize,m_lLastMsgRemainSize);
//__FUNCTION__,
            //�ر���������
            Close();
            return 0x7FFFFFFF;
        }
        if( len < long(4+sizeof(CBaseMessage::stMsg)) )
        {
            //PutErrorString(NET_MODULE,"%-15s Get MsgLen Error3!(flag:%d,Index:%d)m_nReadDataSize:%d,ReadDataBlocksSize:%d,len:%d(m_lLastMsgTotalSize:%d,\
            //             m_lLastMsgLen:%d,m_lLastMsgType:%d,m_lLastMsgPreDBNum:%d,m_lLastMsgPosDBNum:%d,m_lLastMsgMemMoveDBPos:%d,\
            //             m_lLastMsgMemMoveDBSize:%d,m_lLastMsgRemainSize:%d)",
            //             __FUNCTION__,GetFlag(),GetIndexID(),m_nReadDataSize,m_ReadDataBlocks.size(),len,m_lLastMsgTotalSize,m_lLastMsgLen,
            //             m_lLastMsgType,m_lLastMsgPreDBNum,m_lLastMsgPosDBNum,m_lLastMsgMemMoveDBPos,m_lLastMsgMemMoveDBSize,m_lLastMsgRemainSize);
            Close();
            return 0x7FFFFFFF;
        }
    }

    if(size > 0)
    {
        //PutErrorString(NET_MODULE,"%-15s Get MsgLen Error4!(flag:%d,Index:%d)m_nReadDataSize:%d,ReadDataBlocksSize:%d,size:%d(m_lLastMsgTotalSize:%d,\
        //             m_lLastMsgLen:%d,m_lLastMsgType:%d,m_lLastMsgPreDBNum:%d,m_lLastMsgPosDBNum:%d,m_lLastMsgMemMoveDBPos:%d,\
        //             m_lLastMsgMemMoveDBSize:%d,m_lLastMsgRemainSize:%d)",
        //             __FUNCTION__,GetFlag(),GetIndexID(),m_nReadDataSize,m_ReadDataBlocks.size(),size,m_lLastMsgTotalSize,m_lLastMsgLen,
        //             m_lLastMsgType,m_lLastMsgPreDBNum,m_lLastMsgPosDBNum,m_lLastMsgMemMoveDBPos,m_lLastMsgMemMoveDBSize,m_lLastMsgRemainSize);
        len = 0x7FFFFFFF;
    }
    return len;
}

void CServerClient::PushReadDataBack(CDataBlock *pDB)
{
    if(NULL == pDB)	return;
    m_ReadDataBlocks.push_back(pDB);
    m_nReadDataSize += pDB->GetCurSize();
}
CDataBlock *CServerClient::PopReadDataFront()
{
    CDataBlock *pDB = NULL;
    if(m_ReadDataBlocks.size() > 0)
    {
        pDB = m_ReadDataBlocks.front();
        m_ReadDataBlocks.pop_front();
    }
    return pDB;
}
void CServerClient::PushReadDataFront(CDataBlock *pDB)
{
    if(NULL == pDB)	return;
    m_ReadDataBlocks.push_front(pDB);
}

//�����ݿ�ķ�ʽ��������
void CServerClient::OnReceive(int nErrorCode)
{
    if (m_pServers == NULL)	return;
    long recvMsgLen = 0;

    // �ж��Ƿ�������㹻��������
    while ((m_nReadDataSize >= (HEAD_SIZE + sizeof(CBaseMessage::stMsg)) &&
        (recvMsgLen = GetCurMsgLen()) <= m_nReadDataSize))
    {
        char strInfo[512] = "";
        if (recvMsgLen < long(HEAD_SIZE + sizeof(CBaseMessage::stMsg)))
        {
            //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"error��the got data block length size is to small");
            Close();
            return;
        }

        m_lLastMsgTotalSize = m_nReadDataSize;
        m_lLastMsgLen = recvMsgLen;
        m_lLastMsgPreDBNum = (long)m_ReadDataBlocks.size();

        long curMsgRestSize = recvMsgLen;
        vector<CDataBlock*> curMsgDBs;
        curMsgDBs.clear();

        // �������ݵĿ�ʼλ��
        CDataBlock* recvDB = PopReadDataFront();
        long recvDBSize = recvDB->GetCurSize();
        if (recvDBSize == 0)
        {
            //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"���󣬵�������Ϣ�鳤��Ϊ0");
        }

        CDataBlock* allocDB = m_pDBAllocator->AllocDB(9);
        long maxDBSize = allocDB->GetMaxSize();
        long allocDBPos = 0;
        long recvDBPos = 0;
        while (curMsgRestSize > 0 && recvDB && allocDB)
        {
            long minSize = min(min(curMsgRestSize, recvDBSize - recvDBPos),
                maxDBSize - allocDBPos);

            memcpy(allocDB->Base() + allocDBPos, recvDB->Base() + recvDBPos, minSize);
            recvDBPos += minSize;
            allocDBPos += minSize;
            curMsgRestSize -= minSize;

            if(recvDBPos >= recvDBSize && curMsgRestSize > 0)
            {
                m_pDBAllocator->FreeDB(recvDB);
                recvDB = PopReadDataFront();
                if(recvDB )
                {
                    recvDBSize = recvDB->GetCurSize();
                    if(recvDBSize == 0)
                    {
                        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"error��the got data block length size is 0");
                    }
                }
                else
                {
                    //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"error,pop a empty data block!");
                    recvDBSize = 0;
                }
                recvDBPos = 0;
            }

            if(allocDBPos >= maxDBSize && curMsgRestSize > 0)
            {
                allocDB->SetCurSize(maxDBSize);
                curMsgDBs.push_back(allocDB);
                allocDB = m_pDBAllocator->AllocDB(10);
                if(allocDB)
                {
                    maxDBSize = allocDB->GetMaxSize();
                }
                else
                {
                    maxDBSize = 0;
                    //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"error,the pointer allocDB is null");
                }
                allocDBPos = 0;
            }
        }
        allocDB->SetCurSize(allocDBPos);
        curMsgDBs.push_back(allocDB);

        m_lLastMsgRemainSize = curMsgRestSize;
        m_lLastMsgMemMoveDBPos = recvDBPos;
        m_lLastMsgMemMoveDBSize = recvDBSize;

        if (recvDBPos < recvDBSize)
        {
            memmove(recvDB->Base(),
                recvDB->Base() + recvDBPos,
                recvDBSize - recvDBPos);
            recvDB->SetCurSize(recvDBSize-recvDBPos);
            PushReadDataFront(recvDB);
        }
        else if (recvDBPos == recvDBSize)
        {
            m_pDBAllocator->FreeDB(recvDB);
        }

        m_nReadDataSize -= recvMsgLen;
        CBaseMessage* pMsg = CBaseMessage::AllocMsg();
        pMsg->Init(curMsgDBs, m_kn, m_pServers->IsEncryptType(GetFlag()));

        pMsg->SetSocketID(GetIndexID());
        pMsg->SetNetFlag(GetFlag());
        pMsg->SetIP(GetPeerIP());
        pMsg->SetIP(GetDWPeerIP());

        m_lLastMsgType = pMsg->GetType();
        m_lLastMsgPosDBNum = (long)m_ReadDataBlocks.size();
        m_pServers->GetRecvMessages()->PushMessage( pMsg );
    }
}

void CServerClient::OnAccept(int errorCode)
{
    ulong dwPeerIP = GetDWPeerIP();
    CBaseMessage* pMsg = CBaseMessage::AllocMsg();
    pMsg->Init(BASEMSG_Socket_Accept);
    pMsg->SetNetFlag(GetFlag());
    pMsg->SetSocketID(GetIndexID());
    pMsg->SetIP(GetPeerIP());
    pMsg->SetIP(dwPeerIP);
    pMsg->SetTotalSize();
    m_pServers->GetRecvMessages()->PushMessage( pMsg );
}

void CServerClient::OnClose(int errorCode)
{
    CMySocket::OnClose();
    //���߼��㷢�����ӶϿ�����Ϣ
    CBaseMessage* pMsg = CBaseMessage::AllocMsg();
    pMsg->Init(BASEMSG_Socket_Close);
    pMsg->SetNetFlag(GetFlag());
    pMsg->SetSocketID(GetIndexID());
    pMsg->SetIP(GetPeerIP());
    pMsg->SetIP(GetDWPeerIP());
    pMsg->SetTotalSize();
    m_pServers->GetRecvMessages()->PushMessage( pMsg );
}

//�����紫��仯��ʱ��
void CServerClient::OnTransferChange()
{
    //����������Ϣ̫��ʱ
    if(m_SendMessages.size() > m_lMaxBlockSendMsnNum/10)
    {
        //֪ͨ�ϲ㴫��ӵӵ��
        if(!m_bTransfersStatus)
        {
            m_bTransfersStatus = true;
            CBaseMessage* pMsg = CBaseMessage::AllocMsg();
            pMsg->Init(BASEMSG_Transfer_Congestion);
            pMsg->SetNetFlag(GetFlag());
            pMsg->SetSocketID(GetIndexID());
            pMsg->SetIP(GetPeerIP());
            pMsg->SetIP(GetDWPeerIP());
            m_pServers->GetRecvMessages()->PushMessage( pMsg );
        }
    }
    else if(m_SendMessages.size() <= m_lMaxBlockSendMsnNum/100)
    {
        //֪ͨ�ϲ㴫�䳩ͨ
        if(m_bTransfersStatus)
        {
            m_bTransfersStatus = false;
            CBaseMessage* pMsg = CBaseMessage::AllocMsg();
            pMsg->Init(BASEMSG_Transfer_Smoothly);
            pMsg->SetNetFlag(GetFlag());
            pMsg->SetSocketID(GetIndexID());
            pMsg->SetIP(GetPeerIP());
            pMsg->SetIP(GetDWPeerIP());
            m_pServers->GetRecvMessages()->PushMessage( pMsg );
        }
    }
}

long	CServerClient::AddRecvSize(long lSize)
{
    m_pServers->AddRecvSize(GetFlag(),lSize);
    m_lCurTotalRcvSize += lSize;
    //�������ۼƴﵽһ��������ʱ��,����ͳ������
    if( m_lCurTotalRcvSize >= (m_lMaxRcvSizePerSecond<<2) )
    {
        ulong dwCurTime = timeGetTime();
        ulong dwDifTime = dwCurTime - m_dwRcvStartTime;
        if(dwDifTime ==0)	dwDifTime = 1;
        //����ÿ��ƽ�����ܴ�С
        m_lCurRcvSizePerSecond = m_lCurTotalRcvSize*1000/dwDifTime;
        m_dwRcvStartTime = dwCurTime;
        m_lCurTotalRcvSize = 0;
    }
    return m_lCurTotalRcvSize;
}

long CServerClient::AddSendSize(long lSize)
{
    m_pServers->AddSendSize(GetFlag(),lSize);

    ulong dwCurTime = timeGetTime();
    if(dwCurTime - m_dwSendStartTime >= 30000)
    {
        m_dwSendStartTime = dwCurTime;
        m_lCurTotalSendSize = 0;
    }

    m_lCurTotalSendSize += lSize;
    return m_lCurTotalSendSize;
}

//�õ���ǰÿ����ܴ�С
long CServerClient::GetCurRecvSizePerSecond()
{
    return m_lCurRcvSizePerSecond;
}

//�õ���ǰÿ�뷢�ʹ�С
long CServerClient::GetCurSendSizePerSecond()
{
    ulong dwDifTime = timeGetTime()-m_dwSendStartTime;
    if(dwDifTime ==0)	dwDifTime = 1;
    //����ÿ��ƽ�����ܴ�С
    m_lCurSendSizePerSecond = m_lCurTotalSendSize*1000/dwDifTime;
    return m_lCurSendSizePerSecond;
}

void CServerClient::IncUseCount()
{
    if(m_nUseCount >= 65535)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"���������������������ֵ.");
        m_nUseCount = 1;
    }
    m_nUseCount++;
}
