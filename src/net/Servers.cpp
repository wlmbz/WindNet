//=============================================================================
/**
*  file: Servers.cpp
*
*  Brief:��Ҫ�Ƿ�������ʹ�õ������װ����IOCP��ʽʵ�ֵ�����ӿ�
*			�ṩ������������������ӹ���
*
*  Authtor:wangqiao
*	
*	Datae:2005-4-15
*
*	Modify:2007-4-13,�����˴���ͽṹ���Ż���Ч��
*/
//=============================================================================


#include "mmsystem.h"
#include "Servers.h"
#include "Mysocket.h"
#include "serverClient.h"
#include "BaseMessage.h"
#include <time.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CServer::CServer()
{
    m_bAcceptThreadExit=false;
    m_bWorkerThreadsExit=false;

    m_lCurClientCount = 0;

    m_hNetMainTheads=0;
    m_hAcceptThread = 0;	
    m_hWorkerThreads.clear();

    m_bCheck = false;
    m_lForbidTime = 0;	
    m_ForbidIPs.clear();
    m_lMaxBlockConnetNum = SOMAXCONN;
    m_lSendInterTime = 80000;


    m_lMaxClientConNum = 100;			//���Ŀͻ�����������

    m_lPendingWrBufNum = 4096;			//��ÿ���ͻ������ķ��Ͳ�������
    m_lPendingRdBufNum = 2048;
    m_lMaxSendSizePerSecond = 8192;		//���Ƶ�ÿ���ӷ��ʹ�С
    m_lMaxRecvSizePerSecond = 4096;		//���Ƶ�ÿ���ӽ��ܴ�С 
    m_lMaxBlockSendMsgNum = 1024;

    m_lConPendingWrBufNum = 4096;		//��ÿ�����Ӷ����ķ��Ͳ�������
    m_lConPendingRdBufNum = 2048;
    m_lConMaxSendSizePerSecond = 81920;	//���Ӷ�����ÿ���ӷ������ݵĴ�С
    m_lConMaxRecvSizePerSecond = 40960;	//���Ӷ�����ÿ���ӽ������ݵĴ�С
    m_lConMaxBlockSendMsgNum = 1024;

    m_lIOOperDataBufNum = 2;

    m_dwWorkThreadTick=0;			//��ɶ˿ڹ����߳���������
    m_dwNetThreadTick=0;			//�����߳���������
    m_dwAcceptThreadTick=0;		//���������߳���������

    m_NetDataStats.clear();
    m_SendMsgsStat.clear();
    m_RecvMsgsStat.clear();
}

CServer::~CServer()
{	

    CloseHandle( m_hAcceptThread );
    CloseHandle(m_hNetMainTheads);

    for(int i=0;i<(int)m_hWorkerThreads.size();i++)
    {
        CloseHandle(m_hWorkerThreads[i]);
    }


    CloseHandle(m_hCompletionPort);
}

//�������������
//bMode����ģʽ,1:�Կͻ��˵ķ�ʽ����,2:�Է������ķ�ʽ����
bool	CServer::Start(BYTE bMode,CDataBlockAllocator* pDBAllocator,
                       long nMaxFreeSockOperNum,long nMaxFreeIOOperNum,long lIOOperDataBufNum,
                       bool bCheck,long lForbidTime,
                       long lMaxMsgLen,long lMaxConNums,long lTotalClientsNum,
                       long lPendingWrBufNum,long lPendingRdBufNum,
                       long lMaxSendSizePerSecond,long lMaxRecvSizePerSecond,
                       long lMaxBlockSendMsgNum,
                       long lConPendingWrBufNum,long lConPendingRdBufNum,
                       long lConMaxSendSizePerSecond,long lConMaxRecvSizePerSecond,
                       long lConMaxBlockSendMsgNum)
{
    m_bMode = bMode;
    m_pDBAllocator=pDBAllocator;
    m_nMaxFreeSockOperNum = nMaxFreeSockOperNum;
    m_nMaxFreeIOOperNum = nMaxFreeIOOperNum;
    m_lIOOperDataBufNum = lIOOperDataBufNum;

    m_bCheck = bCheck;
    m_lForbidTime = lForbidTime;m_lMaxMessageLength=lMaxMsgLen;
    m_lMaxClientConNum = lMaxConNums;m_lTotalClientsNum = lTotalClientsNum;
    m_lPendingWrBufNum = lPendingWrBufNum;m_lPendingRdBufNum=lPendingRdBufNum;
    m_lMaxSendSizePerSecond = lMaxSendSizePerSecond;m_lMaxRecvSizePerSecond = lMaxRecvSizePerSecond;
    m_lMaxBlockSendMsgNum = lMaxBlockSendMsgNum;
    m_lConPendingWrBufNum = lConPendingWrBufNum;m_lConPendingRdBufNum=lConPendingRdBufNum;
    m_lConMaxSendSizePerSecond= lConMaxSendSizePerSecond;m_lConMaxRecvSizePerSecond = lConMaxRecvSizePerSecond;
    m_lConMaxBlockSendMsgNum = lConMaxBlockSendMsgNum;
    //Ԥ������������ṹ
    InitializeCriticalSection(&m_CSSockOper);
    InitializeCriticalSection(&m_CSMsgStat);

    uint i = 0;
    for(;i<m_nMaxFreeSockOperNum;i++)
    {
        tagSocketOper* pSockOper = new tagSocketOper();
        m_FreeSocketOpers.push_back(pSockOper);
    }
    //Ԥ����IO�����ṹ
    InitializeCriticalSection(&m_CSIOOper);
    i = 0;
    for(;i<m_nMaxFreeIOOperNum;i++)
    {
        PER_IO_OPERATION_DATA* pPerIOData  = new PER_IO_OPERATION_DATA(m_lIOOperDataBufNum);
        m_FreeListIOOpers.push_back(pPerIOData);
    }

    //��ʼ��Ԥ����Ŀͻ���
    InitClients();


    //�õ�CPU������
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    //����cpu����������ɶ˿�
    m_hCompletionPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,sysInfo.dwNumberOfProcessors);

    //������ɶ˿�ʧ��
    if(m_hCompletionPort==NULL)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"������ɶ˿�ʧ��!");
        return false;
    }

    //�����������߳�
    bool bRet = CreateNetMainThread();
    if(!bRet)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�����������߳�ʧ��!");
        return false;
    }

    //��������ɶ˿��ϵȴ��Ĺ������߳�
    bRet=CreateWorkerThreads(sysInfo.dwNumberOfProcessors);	
    if(!bRet)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"���������߳�ʧ��!");
        return false;
    }
    //�ͻ��˷�ʽ����
    if(bMode&0x1)
    {
    }
    //��������ʽ����
    if(bMode&0x2)
    {
        //Host(dwPort,strIP,nSocketType);
    }

    //���ñ���IP
    SetHostName();
    return true;
}

// ���� Listen Socket���������״̬���ȴ��ͻ��˵�����
bool CServer::Host(UINT dwPort, const char* strIP,long lFlag, ulong nSocketType)
{
    if(!(m_bMode&2))	return FALSE;

    SetFlag(lFlag);

    // ���� socket
    bool bRet = CreateEx(dwPort, strIP,nSocketType);	
    //����ʧ��
    if(!bRet)
    {
        return bRet;
    }

    //�������տͻ�socket�߳�
    bRet=CreateAcceptThread();
    if(!bRet)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�������������߳�ʧ��!");

        return FALSE;
    }

    // listen
    bRet = Listen(m_lMaxBlockConnetNum);
    if(!bRet)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"listen ʧ��!");
        return FALSE;
    }
    return bRet;
}

void CServer::InitClients()
{
    InitializeCriticalSection(&m_CSClient);
    //Ԥ�������ӿͻ���
    m_pClients = new CServerClient*[m_lTotalClientsNum];
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        CServerClient *pClient = new CServerClient(this,m_pDBAllocator);
        m_pClients[i] = pClient;
    }
}

void CServer::ReleaseClients()
{
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        CServerClient *pClient =m_pClients[i];
        SAFE_DELETE(pClient);
    }

    delete []m_pClients;
    m_lTotalClientsNum = 0;

    DeleteCriticalSection(&m_CSClient);
}

//�����������߳�
bool CServer::CreateNetMainThread()
{
    g_NewAcceptSockets.clear();
    unsigned threadID;
    m_hNetMainTheads = (HANDLE)_beginthreadex(NULL,0,NetThreadFunc,this,0,&threadID);

    if(m_hNetMainTheads==NULL)
        return FALSE;
    return TRUE;
}
//��������ɶ˿��ϵȴ��Ĺ������߳�
bool CServer::CreateWorkerThreads(int nProcNum)
{
    //	ulong dwThreadId;
    if(nProcNum>=2)
        nProcNum = nProcNum/2;

    for( int i=0;i<nProcNum;i++ )
    {
        unsigned threadID;
        HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL,0,WorkerThreadFunc,this,0,&threadID);

        m_hWorkerThreads.push_back(hWorkerThread);

        if(hWorkerThread==NULL)
            return FALSE;
    }

    PutTraceString(NET_MODULE,"����������%d����ɶ˿ڹ����̡߳�",nProcNum);
    return TRUE;
}


//�������ܿͻ�socket�߳�
bool CServer::CreateAcceptThread()
{
    unsigned threadID;
    m_hAcceptThread = (HANDLE)_beginthreadex(NULL,0,AcceptThreadFunc,this,0,&threadID);

    if(m_hAcceptThread==NULL)
        return FALSE;
    return TRUE;
}

//�˳�������׽���
void CServer::Exit()
{
    ExitWorkerThread();

    //����пͻ���ģʽ,ɾ�������б�
    if(m_bMode&0x1)
    {
    }

    itSockOP itSock = m_FreeSocketOpers.begin();
    for(;itSock != m_FreeSocketOpers.end();itSock++)
    {
        SAFE_DELETE((*itSock));
    }
    DeleteCriticalSection(&m_CSSockOper);
    DeleteCriticalSection(&m_CSMsgStat);

    itIOOper itIo = m_FreeListIOOpers.begin();
    for(;itIo != m_FreeListIOOpers.end();itIo++)
    {
        SAFE_DELETE((*itIo));
    }
    DeleteCriticalSection(&m_CSIOOper);

    //�ͷ�Serveclient����
    ReleaseClients();

    //�ͷ���Ϣ���������Ϣ
    CBaseMessage* pMsg = NULL;
    while(pMsg = GetRecvMessages()->PopMessage())
    {
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
}

//�����˳���ɶ˿ڹ������̱߳�־
void CServer::ExitWorkerThread(void)
{
    //�����߳��˳�
    //m_bAcceptThreadExit=true;
    //WaitForSingleObject(m_hAcceptThread,INFINITE);
    //����Ƿ�����ģʽ���ر������߳�
    if(m_bMode&0x2)
    {
        TerminateThread(m_hAcceptThread,0);
    }

    CloseAll();

    while(m_lCurClientCount>0)
    {
        Sleep(100);
    }


    //��ɶ˿ڵĹ����߳��˳�
    int nSize = (int)m_hWorkerThreads.size();
    for(int i=0;i < nSize; i++)
    {
        //���������̷߳����˳���Ϣ
        PostQueuedCompletionStatus(m_hCompletionPort,0,0,0);
    }

    WaitForMultipleObjects((ulong)m_hWorkerThreads.size(),&m_hWorkerThreads[0],TRUE,INFINITE);	


    //�����˳����̲߳�������
    tagSocketOper* pSocketOpera = AllocSockOper();
    pSocketOpera->Init(SCOT_ExitThread,0,0);
    m_SocketOperaCommands.Push_Back(pSocketOpera);
    //�Ⱥ��������߳����
    WaitForSingleObject(m_hNetMainTheads,INFINITE);
}


tagSocketOper* CServer::AllocSockOper()
{
    tagSocketOper* pSockOper = NULL;

    EnterCriticalSection(&m_CSSockOper);
    if(m_FreeSocketOpers.size() > 0)
    {
        pSockOper = m_FreeSocketOpers.front();
        m_FreeSocketOpers.pop_front();
    }
    LeaveCriticalSection(&m_CSSockOper);

    if(pSockOper == NULL)
        pSockOper = new tagSocketOper();
    return pSockOper;
}


void CServer::FreeSockOper(tagSocketOper* pSockOper)
{
    if(pSockOper == NULL)	return;
    EnterCriticalSection(&m_CSSockOper);
    if(m_FreeSocketOpers.size() < m_nMaxFreeSockOperNum)
    {
        m_FreeSocketOpers.push_back(pSockOper);
    }
    else
    {
        SAFE_DELETE(pSockOper);
    }
    LeaveCriticalSection(&m_CSSockOper);
}

PER_IO_OPERATION_DATA* CServer::AllocIoOper()
{
    PER_IO_OPERATION_DATA* pPerIOData = NULL;
    EnterCriticalSection(&m_CSIOOper);
    if(m_FreeListIOOpers.size() > 0)
    {
        pPerIOData = m_FreeListIOOpers.front();
        m_FreeListIOOpers.pop_front();
    }
    LeaveCriticalSection(&m_CSIOOper);

    if(NULL == pPerIOData)
        pPerIOData = new PER_IO_OPERATION_DATA(m_lIOOperDataBufNum);
    return pPerIOData;
}

void CServer::FreeIoOper(PER_IO_OPERATION_DATA* pPerIOData)
{
    if(pPerIOData == NULL)	return;

    pPerIOData->pParam = NULL;
    EnterCriticalSection(&m_CSIOOper);
    if(m_FreeListIOOpers.size() < m_nMaxFreeIOOperNum)
    {
        if(m_lIOOperDataBufNum > pPerIOData->m_lDataBufNum)
            pPerIOData->ResetDataBufSize(m_lIOOperDataBufNum);
        m_FreeListIOOpers.push_back(pPerIOData);
    }
    else
    {
        SAFE_DELETE(pPerIOData);
    }
    LeaveCriticalSection(&m_CSIOOper);
}

// [TCP] ����������رյ�����
void CServer::OnClose(int nErrorCode)
{
    CMySocket::OnClose(nErrorCode);
}

//Server��������ɶ˿�ģʽ��ʱ��ʹ�ø÷�������OnAccept
void CServer::OnAccept(int nErrorCode)
{

    char strInfo[256]="";
    SOCKET clientSocket;
    struct sockaddr_in stClientAddress;
    int nClientAddressLen=sizeof(stClientAddress);    

    clientSocket = WSAAccept (m_hSocket,
        (struct sockaddr*)&stClientAddress,
        &nClientAddressLen,
        NULL,
        0);

    if(clientSocket==INVALID_SOCKET)
    {
        ulong dwEorror = WSAGetLastError();
        return;
    }

    //�ж��Ƿ񳬹����������
    if(m_lCurClientCount >= m_lMaxClientConNum)
    {
        closesocket(clientSocket);
        PutTraceString(NET_MODULE,"the cur connect num(%d) greater the restrict num(%d)",m_lCurClientCount,m_lMaxClientConNum);

        return;
    }

    //�쿴��IP�Ƿ񱻽�ֹ
    if(m_bCheck && FindForbidIP(stClientAddress.sin_addr.S_un.S_addr) == true)
    {
        closesocket(clientSocket);
        Log4c::Warn(NET_MODULE,"the ip address(%d) has already forbided",stClientAddress.sin_addr.S_un.S_addr);
        return;
    }

    //����
    CServerClient* pServerClient=  AllocClient(true);
    if(NULL == pServerClient)
    {
        Log4c::Warn(NET_MODULE,"erro, Don't allocate the 'serverclient' instance.��");
        closesocket(clientSocket);
        return;
    }

    pServerClient->m_hSocket=clientSocket;
    pServerClient->SetFlag(GetFlag());
    /// ���üӽ�������Key.
    static bool bFirst = true;
    if (IsEncryptType(pServerClient->GetFlag()))
    {
        if (bFirst)
        {
            bFirst = false;
            struct sockaddr_in addr;
            int len = sizeof(addr);
            getsockname(clientSocket, (struct sockaddr *)&addr, &len);
            SetKey(addr.sin_addr.S_un.S_addr, addr.sin_port);
        }
        pServerClient->GetKn(m_kn);
    }

    //����IP
    pServerClient->SetPeerIP(inet_ntoa(stClientAddress.sin_addr));
    pServerClient->SetPeerIP(stClientAddress.sin_addr.S_un.S_addr);
    pServerClient->SetPeerPort(stClientAddress.sin_port);
    pServerClient->SetParam(GetCheck(),GetMaxMsgLen(),m_lMaxBlockSendMsgNum,
        m_lPendingWrBufNum,m_lPendingRdBufNum,
        m_lMaxSendSizePerSecond,m_lMaxRecvSizePerSecond,0);
    pServerClient->SetClose(false);
    pServerClient->SetShutDown(false);

    //����Socket��indexID
    pServerClient->SetSendRevBuf();
    //�����׽���ѡ��
    pServerClient->SetSocketOpt();

    tagSocketOper *pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_Init,pServerClient->GetIndexID(),pServerClient);
    m_SocketOperaCommands.Push_Back(pSocketOper);

    pServerClient->OnAccept();

    IncAcceptThreadTick();
}

// error��ʱ�����
void CServer::OnError(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,int errorCode)
{
    if(NULL == pPerIOData)	return;
    char str[200];
    if(pPerIOData->OperationType == SOT_Send)
    {
        sprintf(str, "��ɶ˿��̲߳���һ��ʧ�ܵķ���IO����(ErrorID:%d)��",errorCode);
        CBaseMessage* pMsg = (CBaseMessage*)pPerIOData->pParam;
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
    else if(pPerIOData->OperationType == SOT_Receive)
    {
        sprintf(str, "��ɶ˿��̲߳���һ��ʧ�ܵĽ���IO����(ErrorID:%d)��",errorCode);
        m_pDBAllocator->FreeDB((CDataBlock*)pPerIOData->pParam);
    }


    CServerClient *pClient = FindClient(lIndexID);
    if(pClient)
    {
        //ȡ����ʱ����
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(pClient->GetIndexID());
        pClient->OnClose();
        FreeClient(pClient);
    }
    FreeIoOper(pPerIOData);

    if(errorCode != 64)
    {
       PutTraceString(NET_MODULE,str);
    }
}

//���رյ�ʱ��
void CServer::OnClose(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID)
{
    if(NULL == pPerIOData)	return;

    if(pPerIOData->OperationType == SOT_Send)
    {
        CBaseMessage* pMsg = (CBaseMessage*)pPerIOData->pParam;
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
    else if(pPerIOData->OperationType == SOT_Receive)
    {
        m_pDBAllocator->FreeDB((CDataBlock*)pPerIOData->pParam);
    }

    CServerClient *pClient = FindClient(lIndexID);
    if(pClient)
    {
        //ȡ����ʱ����
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(pClient->GetIndexID());
        pClient->OnClose();
        FreeClient(pClient);
    }
    FreeIoOper(pPerIOData);
}

//����ɶ˿��Ϸ������ݽ���
void CServer::OnSend(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lSendNum)
{
    if(NULL == pPerIOData)	return;

    CBaseMessage *pMsg = (CBaseMessage*)pPerIOData->pParam;
    //long lMsgType = pMsg->GetType();
    //long lMsgSize = pMsg->GetTotalSize();	
    if(pMsg->RemoveRefCount() == 0)
        CBaseMessage::FreeMsg(pMsg);
    FreeIoOper(pPerIOData);

    CServerClient* pClient = FindClient(lIndexID);
    long lFlag = -1;
    if(pClient)
    {
        lFlag = pClient->GetFlag();
        pClient->DecPendingWrBufNum(lSendNum);
        //�������Է�����Ϣ
        pClient->Send(NULL,0,0);

        //pClient->OnTransferChange();
    }
}


void CServer::OnSendZeorByte(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID)
{
    if(NULL == pPerIOData)	return;
    FreeIoOper(pPerIOData);
    CServerClient* pClient = FindClient(lIndexID);
    if(pClient)
    {
        pClient->OnSendZeroByteData();
        //�������Է�����Ϣ
        pClient->Send(NULL,0,0);
    }
}

void CServer::OnDisConn(long lIndexID)
{
    CServerClient* pClient = FindClient(lIndexID);
    if(pClient)
    {
        pClient->ShutDown();
    }	
}

void CServer::OnReceive(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lReadNum)
{
    CServerClient* pClient = FindClient(lIndexID);	

    if( pClient )
    {
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(lIndexID);
        CDataBlock *pReadDB = (CDataBlock*)pPerIOData->pParam;

        //��Ӷ����ݵĴ�С
        pClient->AddRecvSize(lReadNum);
        //���ö�ȡ���ݵĴ�С
        pReadDB->SetCurSize(lReadNum);
        //�����ɵĶ�����
        pClient->AddReceiveData(pPerIOData->m_nSequenceNumber,pReadDB);
        //���¿�ʼ��ȡ����
        if(pClient->ReadFromCompletionPort(pPerIOData,NULL) )
        {
            pClient->OnReceive();
            //���ƽ��ÿ����ܵĴ�С��������ֵ,����������Ϣ.�Ͽ�
            if(pClient->GetCurRecvSizePerSecond() > pClient->GetMaxRecvSizePerSecond() )
            {				
                Log4c::Warn(NET_MODULE,"erro,(SocketID:%d) the current rcv data size per-second(%d) greater the max value(%d),shutdown the socket!",
                    lIndexID,pClient->GetCurRecvSizePerSecond(),pClient->GetMaxRecvSizePerSecond());
                //�ر�
                pClient->Close();;
            }
        }		
        else
        {
            //����������
            FreeIoOper(pPerIOData);
            pClient->OnClose();
            FreeClient(pClient);
        }		
    }
    else
    {
        //����������
        FreeIoOper(pPerIOData);
    }
    return;
}

// [UDP] �ڽ��յ���Ϣʱ����
void CServer::OnReceive(int nErrorCode)
{
}

//�첽������Ϣ
int CServer::ASend(long lIndexID,CBaseMessage *pMsg)
{
    if(NULL == pMsg)	return false;
    //���÷���ʱ��,���ڼ�����Ϣ�����ȼ�
    pMsg->SetStartSendTime(timeGetTime());
    tagSocketOper *pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_Send,lIndexID,(void*)pMsg);
    m_SocketOperaCommands.Push_Back(pSocketOper);
    return true;
}

// ���ڰһ��SOCKET����ɶ˿�
bool CServer::AssociateSocketWithCP(SOCKET socket,ulong dwCompletionKey)
{
    if(m_hCompletionPort == NULL)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�ں��� CServer::AssociateSocketWithCP(...)�����������ɶ˿ھ����Ч��");
        return false;
    }
    HANDLE hHanle = CreateIoCompletionPort((HANDLE)socket, m_hCompletionPort, dwCompletionKey, 0);
    if(hHanle == NULL)
    {
        ulong dwErrorID = GetLastError();
        PutErrorString(NET_MODULE,"%-15s�ں��� CServer::AssociateSocketWithCP(...)��.�һ��Socket����ɶ˿ڳ���(ErrorID:%d)",__FUNCTION__,dwErrorID);
        return false;
    }
    if(hHanle!= m_hCompletionPort)
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�ں��� CServer::AssociateSocketWithCP(...)����������SOCKET����ɶ˿ڵķ���ֵ����");
        return false;
    }
    return true;
}

// [������]
bool CServer::Listen(int nConnectionBacklog)
{
    // [TCP]
    if( GetProtocol() == SOCK_STREAM )
    {
        if(SOCKET_ERROR != listen(m_hSocket, nConnectionBacklog))
        {
            return TRUE;
        }
    }
    return FALSE;
}


long CServer::OnDisConnAll()
{
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == true )
        {
            m_pClients[i]->ShutDown();
        }
    }
    LeaveCriticalSection(&m_CSClient);
    return true;
}

long CServer::OnCloseAll()
{
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == true )
        {
            m_pClients[i]->Close();
        }
    }
    //m_lCurClientCount = 0;
    LeaveCriticalSection(&m_CSClient);
    return true;
}

void	CServer::AddForbidIP(u_long laddr)
{
    m_ForbidIPs[laddr] = timeGetTime();
}

bool	CServer::FindForbidIP(u_long laddr)
{
    map<u_long,ulong>::iterator it = m_ForbidIPs.find(laddr);
    if(it != m_ForbidIPs.end())
    {
        ulong dwTime = timeGetTime();
        if(dwTime-(*it).second <= (ulong)m_lForbidTime)
        {
            return true;
        }
        m_ForbidIPs.erase(laddr);
    }
    return false;
}


long	CServer::AddSendSize(long lFlagID,long lSize)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it == m_NetDataStats.end() )
    {
        tagDataStat dataState;
        memset(&dataState,0,sizeof(tagDataStat));
        dataState.dwSendStartTime = dataState.dwRecvStartTime = timeGetTime();
        m_NetDataStats[lFlagID] = dataState;
        it = m_NetDataStats.find(lFlagID);
    }	
    tagDataStat &dataState = (*it).second;	
    dataState.lTotalSendSize += lSize;
    return dataState.lTotalSendSize;
}

long	CServer::AddRecvSize(long lFlagID,long lSize)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it == m_NetDataStats.end() )
    {
        tagDataStat dataState;
        memset(&dataState,0,sizeof(tagDataStat));
        dataState.dwSendStartTime = dataState.dwRecvStartTime = timeGetTime();
        m_NetDataStats[lFlagID] = dataState;
        it = m_NetDataStats.find(lFlagID);
    }
    tagDataStat &dataState = (*it).second;
    dataState.lTotalRecvSize += lSize;
    return dataState.lTotalRecvSize;
}
long	CServer::GetSendSizePerSecond(long lFlagID)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it != m_NetDataStats.end() )
    {
        tagDataStat &dataState = (*it).second;
        ulong dwCurTime = timeGetTime();
        ulong DifTime = dwCurTime-dataState.dwSendStartTime;
        if(DifTime == 0) DifTime = 1;
        long lSendSizePerSecond = dataState.lTotalSendSize*1000/DifTime;
        dataState.dwSendStartTime = dwCurTime;
        dataState.lTotalSendSize = 0;
        return lSendSizePerSecond;
    }
    return 0;
}

long	CServer::GetRecvSizePerSecond(long lFlagID)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it != m_NetDataStats.end() )
    {
        tagDataStat &dataState = (*it).second;
        ulong dwCurTime = timeGetTime();
        ulong DifTime = dwCurTime-dataState.dwRecvStartTime;
        if(DifTime == 0) DifTime = 1;

        long lRecvSizePerSecond = dataState.lTotalRecvSize*1000/DifTime;
        dataState.dwRecvStartTime = dwCurTime;
        dataState.lTotalRecvSize = 0;
        return lRecvSizePerSecond;
    }
    return 0;
}


//��ӷ�����Ϣ����ͳ��
void	CServer::AddSendMsgStat(long lMsgType,long lSize)
{
    EnterCriticalSection(&m_CSMsgStat);
    itMsgStat it = m_SendMsgsStat.find(lMsgType);
    if(it == m_SendMsgsStat.end())
    {
        tagMsgStat msgStat = {0,1};
        m_SendMsgsStat.insert(make_pair(lMsgType,msgStat));
        it = m_SendMsgsStat.find(lMsgType);
    }
    tagMsgStat &msgStat = (*it).second;
    msgStat.lNum++;
    msgStat.lTotalSize += lSize;

    itMaxMsgLen itMaxLen = m_SendMsgMaxLen.find(lMsgType);
    if(itMaxLen != m_SendMsgMaxLen.end() )
    {
        if((*itMaxLen).second < lSize)
            (*itMaxLen).second = lSize;
    }
    else
        m_SendMsgMaxLen[lMsgType]=lSize;

    LeaveCriticalSection(&m_CSMsgStat);

}
//��ӽ�����Ϣ����ͳ��
void	CServer::AddRecvMsgStat(long lMsgType,long lSize)
{
    EnterCriticalSection(&m_CSMsgStat);
    itMsgStat it = m_RecvMsgsStat.find(lMsgType);
    if(it == m_RecvMsgsStat.end())
    {
        tagMsgStat msgStat = {0,1};
        m_RecvMsgsStat.insert(make_pair(lMsgType,msgStat));
        it = m_RecvMsgsStat.find(lMsgType);
    }
    tagMsgStat &msgStat = (*it).second;
    msgStat.lNum++;
    msgStat.lTotalSize += lSize;

    itMaxMsgLen itMaxLen = m_RecvMsgMaxLen.find(lMsgType);
    if(itMaxLen != m_RecvMsgMaxLen.end() )
    {
        if((*itMaxLen).second < lSize)
            (*itMaxLen).second = lSize;
    }
    else
        m_RecvMsgMaxLen[lMsgType]=lSize;

    LeaveCriticalSection(&m_CSMsgStat);
}


//�����Ϣͳ����Ϣ��net.log�ļ�
void	CServer::OutputMsgStatInfo(ulong lIntelTime)
{
    static char pszInfo[1024]="";
    static ulong dwLastTime = timeGetTime();	
    ulong dwCurTiem  = timeGetTime();
    if(dwCurTiem-dwLastTime < lIntelTime)
        return;
    dwLastTime = dwCurTiem;

    EnterCriticalSection(&m_CSMsgStat);
    PutTraceString(NET_MODULE,"the send msgs info:");
    itMsgStat it = m_SendMsgsStat.begin();
    for(;it != m_SendMsgsStat.end();it++)
    {
       PutTraceString(NET_MODULE,"msgType:%d,count:%d,totalsize:%d",(*it).first,(*it).second.lNum,(*it).second.lTotalSize);
    }
    m_SendMsgsStat.clear();

   PutTraceString(NET_MODULE,"the recv msgs info:");
    it = m_RecvMsgsStat.begin();
    for(;it != m_RecvMsgsStat.end();it++)
    {
        PutTraceString(NET_MODULE,"msgType:%d,count:%d,totalsize:%d",(*it).first,(*it).second.lNum,(*it).second.lTotalSize);
    }
    m_RecvMsgsStat.clear();

    PutTraceString(NET_MODULE,"the msgs send max len:");
    itMaxMsgLen itMaxLen = m_SendMsgMaxLen.begin();
    for(;itMaxLen != m_SendMsgMaxLen.end();itMaxLen++)
    {
        PutTraceString(NET_MODULE,"msgType:%d,maxLen:%d",(*itMaxLen).first,(*itMaxLen).second);
    }

    PutTraceString(NET_MODULE,"the msgs recv max len:");
    itMaxLen = m_RecvMsgMaxLen.begin();
    for(;itMaxLen != m_RecvMsgMaxLen.end();itMaxLen++)
    {
        PutTraceString(NET_MODULE,"msgType:%d,maxLen:%d",(*itMaxLen).first,(*itMaxLen).second);
    }

    LeaveCriticalSection(&m_CSMsgStat);
}

void	CServer::AddNewAcceptSocket(ulong dwSocketID)
{
    ulong dwTime = timeGetTime();
    g_NewAcceptSockets[dwSocketID] = dwTime;
}
void	CServer::RemoveNewAcceptSocket(ulong dwSocketID)
{
    g_NewAcceptSockets.erase(dwSocketID);
}
void	CServer::DoNewAcceptSocket()
{
    ulong dwTime = timeGetTime();
    map<ulong,ulong>::iterator it = g_NewAcceptSockets.begin();
    for(;it != g_NewAcceptSockets.end();)
    {
        if( dwTime-(*it).second >= m_lSendInterTime )
        {
            OnDisConn((*it).first );
            it = g_NewAcceptSockets.erase(it);
        }
        else
        {
            it++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

CServerClient*	CServer::AllocClient(bool bAccept)
{
    CServerClient *pClient = NULL;
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == false )
        {
            pClient = m_pClients[i];
            pClient->Init();

            pClient->SetUsing(true);
            pClient->SetIsAccpet(bAccept);
            pClient->IncUseCount();
            pClient->SetIndexID( i<<16|pClient->GetUseCount() );
            m_lCurClientCount++;
            break;
        }
    }
    LeaveCriticalSection(&m_CSClient);
    return pClient;
}
void CServer::FreeClient(CServerClient* pClient)
{
    if(NULL == pClient)	return;
    EnterCriticalSection(&m_CSClient);
    pClient->Release();
    pClient->SetUsing(false);
    pClient->SetTransCong(false);
    pClient->SetIndexID(0);

    LeaveCriticalSection(&m_CSClient);
    m_lCurClientCount--;
}

// ����һ���ͻ��ˣ��������꣬-1��ʾû��
CServerClient* CServer::FindClient(long lSocketID)
{
    long lIndex = lSocketID >> 16;
    if(lIndex >= m_lTotalClientsNum || lSocketID <= 0 )	return NULL;
    CServerClient* pClient = m_pClients[lIndex];
    if(pClient->GetIndexID() != lSocketID)	return NULL;

    return pClient;
}

// ɾ��һ���ͻ���
bool CServer::FreeClient(long lSocketID)
{
    CServerClient* pClient = FindClient(lSocketID);
    FreeClient(pClient);	
    return true;	
}

//�������ӷ�����,���ظ����ӵ�ID
//����-1����ʾʧ��
long CServer::Connect(LPCTSTR lpszHostAddress, UINT nHostPort,long lFlag,ulong dwTimeOut)
{
    if(!(m_bMode&1))	return -1;

    CServerClient* pConClient = AllocClient(false);
    if(pConClient == NULL)	return -1;

    if( !pConClient->CreateEx(0,NULL,SOCK_STREAM) )
    {
        PutErrorString(NET_MODULE,"%-15s Create socket bound to %s:%u FAILED, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, GetLastError());
        return -1;
    }

    pConClient->SetFlag(lFlag);
    //���ñ��
    pConClient->SetSendRevBuf();
    //�����׽���ѡ��
    pConClient->SetSocketOpt();

    long lID = pConClient->GetIndexID();

    pConClient->SetClose(false);
    pConClient->SetShutDown(false);

    pConClient->SetParam(false,0xFFFFFFF,m_lConMaxBlockSendMsgNum,
        m_lConPendingWrBufNum,m_lConPendingRdBufNum,
        m_lConMaxSendSizePerSecond,m_lConMaxRecvSizePerSecond,lFlag);

    if( !Connect(pConClient,lpszHostAddress,nHostPort,dwTimeOut) )
    {
        pConClient->Close();
        FreeClient(pConClient);
        return -1;
    }

    tagSocketOper* pSocketOpera = AllocSockOper();
    pSocketOpera->Init(SCOT_Init,lID,pConClient,0);
    m_SocketOperaCommands.Push_Back(pSocketOpera);

    return lID;
}

long CServer::Connect(CServerClient* pConClient,LPCTSTR lpszHostAddress, UINT nHostPort,ulong dwTimeOut)
{
    if(NULL == pConClient || NULL == pConClient->m_hSocket)	return FALSE;

    //�������ӵķ�����IP�Ͷ˿�
    pConClient->SetPeerIP(lpszHostAddress);
    pConClient->SetPeerPort(nHostPort);

    SOCKADDR_IN sockAddr;
    memset(&sockAddr,0,sizeof(sockAddr));
    LPSTR lpszAscii = (LPTSTR)lpszHostAddress;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
    if (sockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        PutErrorString(NET_MODULE,"%-15s Given address %s:%u is INVALID, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, GetLastError());
        return FALSE;
    }

    sockAddr.sin_port = htons((u_short)nHostPort);
    if (IsEncryptType(pConClient->GetFlag()))
    {
        pConClient->SetKey(sockAddr.sin_addr.s_addr, sockAddr.sin_port);
    }

    int ret = connect(pConClient->m_hSocket,(SOCKADDR*)&sockAddr, sizeof(sockAddr));
    if( ret == SOCKET_ERROR )
    {
        ulong dwError = WSAGetLastError();
        if(  dwError != WSAEWOULDBLOCK )
        {
            return FALSE;
        }

        //�Ⱥ�һ��ʱ��,���Ƿ��д���ж����ӳɹ����
        fd_set writefd;
        FD_ZERO(&writefd);
        FD_SET(pConClient->m_hSocket,&writefd);
        timeval waittime = {dwTimeOut/1000,(dwTimeOut%1000)*1000};
        int nRet = select(0,NULL,&writefd,NULL,&waittime);
        if(nRet == 0 || nRet == SOCKET_ERROR)
        {
            PutErrorString(NET_MODULE,"%-15s Select %s:%u FAILED, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, WSAGetLastError());
            return FALSE;
        }
    }
    return	TRUE;
}

//�����ر�����
bool  CServer::DisConn(long lIndexID)
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_DisConn,lIndexID,NULL);
    m_SocketOperaCommands.Push_Back(pSocketOper);
    return true;
}

bool  CServer::DisConnAll()
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_DisConnAll,0,NULL);
    m_SocketOperaCommands.Push_Back(pSocketOper);
    return true;
}

bool  CServer::CloseAll()
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_CloseAll,0,NULL);
    m_SocketOperaCommands.Push_Back(pSocketOper);
    return true;
}


bool CServer::OnClientInit(CServerClient* pClient)
{
    if(pClient == NULL)	return false;

    long lIndexID = pClient->GetIndexID();


    //���׽��ֵ���ɶ˿�
    if( !AssociateSocketWithCP(pClient->m_hSocket,lIndexID) )
    {
        pClient->Close();
        FreeClient(pClient);
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"�ں���CServer::OnConClientInitial(...)��,AssociateSocketWithCP(...)����ʧ��.");
        return false;
    }

    if( pClient->StartReadData() == false)
    {
        pClient->Close();
        FreeClient(pClient);
        return false;
    }

    if( pClient->IsAccept() )
        AddNewAcceptSocket(lIndexID);

    //�жϽ��������Ƿ�ʱ����ʱ��رո��׽���
    DoNewAcceptSocket();
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void DoWorkerThreadFunc(CServer* pServer)
{
    bool bResult;//completion port packet flag
    ulong dwNumRead;//bytes num be readed
    ULONG CPDataKey;
    LPER_IO_OPERATION_DATA lpPerIOData;

    char strTempt[100]="";
    while(true)
    {
        //pServer->IncWorkThreadTick();
        //�ȴ���ɶ˿�ͨ��
        dwNumRead = -1;
        bResult=GetQueuedCompletionStatus(pServer->m_hCompletionPort,
            &dwNumRead,
            &CPDataKey,
            (LPOVERLAPPED*)&lpPerIOData,
            INFINITE)
            ? true : false;

        if(!bResult)
        {
            if(lpPerIOData == NULL && dwNumRead == -1)
            {
                PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__, "��ɶ˿��̲߳�������,δȡ����ɰ�(bResult == false,lpPerIOData == NULL,ErrorID:%d)��",GetLastError());
            }
            else if(lpPerIOData)
            {
                ulong dwError = GetLastError();
                //���һ��ɾ��Client�Ĳ�������
                tagSocketOper *pSocketOper = pServer->AllocSockOper();
                pSocketOper->Init(SCOT_OnError,CPDataKey,(void*)lpPerIOData,dwError);
                pServer->m_SocketOperaCommands.Push_Back(pSocketOper);
            }
            else
            {
               PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"��ɶ˿��̲߳���δ֪����");
            }
        }
        else  ////data is valid,dispose!
        {	
            //�˳�
            if(CPDataKey==0)
            {
                break;
            }
            else if(lpPerIOData == NULL)
            {
                PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"��ɶ˿��̲߳�������(bResult == true,lpPerIOData == NULL)��");
            }
            else
            {
                tagSocketOper *pSocketOper = pServer->AllocSockOper();
                if(lpPerIOData->OperationType == SOT_SendZeroByte)
                {
                    //����Ƿ���0�ֽڴ�С�Ĳ���
                    //���һ��ɾ��Client�Ĳ�������					
                    pSocketOper->Init(SCOT_OnSendZeroByte,CPDataKey,(void*)lpPerIOData,0);
                }
                else if(dwNumRead==0)
                {
                    //���һ��ɾ��Client�Ĳ�������
                    pSocketOper->Init(SCOT_OnClose,CPDataKey,(void*)lpPerIOData,0);
                }
                else if(lpPerIOData->OperationType == SOT_Receive)
                {
                    pSocketOper->Init(SCOT_OnRecieve,CPDataKey,(void*)lpPerIOData,dwNumRead);
                }
                else if(lpPerIOData->OperationType == SOT_Send)
                {		
                    //���һ����ɷ��͵���Ϣ
                    pSocketOper->Init(SCOT_OnSend,CPDataKey,(void*)lpPerIOData,dwNumRead);
                }
                pServer->m_SocketOperaCommands.Push_Back(pSocketOper);
            }
        }
    }
}

//*****************************************
//[������]�������̵߳��߼�����
//*****************************************
unsigned __stdcall WorkerThreadFunc(void* pArguments)
{
    CServer* pServer=(CServer*)pArguments;

#ifndef _DEBUG
    __try
    {
#endif

        DoWorkerThreadFunc(pServer);

#ifndef _DEBUG
    }
    __except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"********��ɶ˿ڹ����̳߳�����鿴���µĴ��󱨸��ļ�********");
    }
#endif

    _endthreadex( 0 );
    return 0;
}

unsigned __stdcall AcceptThreadFunc(void* pArguments)
{	
    CServer* pServer=(CServer*)pArguments;

    if(pServer==NULL)
    {
        _endthreadex( 0 );
        return 0;
    }

#ifndef _DEBUG
    __try
    {
#endif
        //����client socket
        while(true)
        {	
            pServer->OnAccept(0);
        }

#ifndef _DEBUG
    }
    __except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"********�����������̳߳�����鿴���µĴ��󱨸��ļ�********");

    }
#endif

    _endthreadex( 0 );
    return 0;
}

void DoNetThreadFunc(CServer* pServer)
{
    char strTempt[200]="";
    CommandsQueue TemptCommands;
    //��ȡһ����������
    bool bRun = true;
    while(bRun)
    {
        pServer->IncNetThreadTick();
        TemptCommands.clear();
        pServer->m_SocketOperaCommands.CopyAllCommand(TemptCommands);
        CommandsQueue::iterator itCommand = TemptCommands.begin();
        for(; itCommand != TemptCommands.end();itCommand++)
        {
            tagSocketOper* pSocketOper = (*itCommand);

            if(pSocketOper)
            {
                switch(pSocketOper->OperaType)
                {
                    //��ʼ������
                case SCOT_Init:
                    {
                        CServerClient* pClient = (CServerClient*)(pSocketOper->pBuf);
                        pServer->OnClientInit(pClient);
                    }
                    break;
                    //�����Ͽ�ĳ������
                case SCOT_DisConn:
                    {
                        pServer->OnDisConn(pSocketOper->lSocketID);
                    };
                    break;
                case SCOT_DisConnAll:
                    {	//ɾ�����пͻ���
                        pServer->OnDisConnAll();
                    }
                    break;
                case SCOT_CloseAll:
                    {
                        pServer->OnCloseAll();
                    }
                    break;
                case SCOT_Send:
                    {
                        CServerClient* pClient = pServer->FindClient(pSocketOper->lSocketID);
                        if(pClient)
                        {
                            pClient->AddSendMsg((CBaseMessage*)pSocketOper->pBuf);
                        }
                        else
                        {
                            CBaseMessage* pMsg = (CBaseMessage*)pSocketOper->pBuf;
                            if(pMsg->RemoveRefCount() == 0)
                                CBaseMessage::FreeMsg(pMsg);
                        }
                    }
                    break;
                case SCOT_OnSend:
                    {
                        pServer->OnSend((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    }
                    break;
                case SCOT_OnSendZeroByte:
                    {
                        pServer->OnSendZeorByte((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID);
                    }
                    break;
                case SCOT_OnRecieve:
                    {
                        pServer->OnReceive((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    }
                    break;
                case SCOT_OnClose:
                    {
                        pServer->OnClose((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID);
                    };
                    break;
                case SCOT_OnError:
                    {
                        pServer->OnError((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    };
                    break;
                case SCOT_ExitThread:
                    {
                        bRun = false;
                    }
                    break;
                }
                pServer->FreeSockOper(pSocketOper);
            }
        }

        TemptCommands.clear();
        //Sleep(1);
    }
}

//�������̣߳�������Ĳ���������н�����ִ��
unsigned __stdcall NetThreadFunc(void* pArguments)
{
    CServer* pServer=(CServer*)pArguments;
    if(pServer==NULL)
    {
        _endthreadex( 0 );
        return 0;
    }

#ifndef _DEBUG
    __try
    {
#endif

        DoNetThreadFunc(pServer);

#ifndef _DEBUG
    }
    __except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"********�����������̳߳�����鿴���µĴ��󱨸��ļ�********");
    }
#endif

    _endthreadex( 0 );
    return 0;
}
