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

#pragma once

#include <list>
#include <vector>
#include <map>
#include "Servers.h"

class CBaseMessage;

/*-------------------------------------------------------------------
[CServerClient] ��������Ӧÿ���ͻ��˵�����

���ܣ�
������TCPЭ�飬UDPЭ�鲻��Ҫ���ࡣ

--------------------------------------------------------------------*/

#define MAXIMUMSEQUENSENUMBER 10000

class CServerClient : public CMySocket
{
protected:	
    bool m_bLost;						// �Ƿ�����˵ı�־
    bool m_bQuit;						// �׽����Ƿ��˳�
    bool m_bAccept;						// �Ƿ���Accept�������׽���

    bool m_bSendZeroByteData;			// �Ƿ��Ѿ�������0�ֽڴ�С������

    bool m_bTransfersStatus;			//���͵Ĵ���״��,true:��ʾӵ��,false:��ʾ��ͨ
    //�Ƿ�����ʹ����
    bool m_bUsing;
    //ʹ�ü���
    ushort m_nUseCount;

    long m_bServerType;                 //�ͻ������� GameServer Or LogingServer

    CServer*		m_pServers;			//���Ƶĸ��׽���
    CDataBlockAllocator* m_pDBAllocator;

    typedef std::list<CBaseMessage*>	 NetMessages;
    typedef std::list<CBaseMessage*>::iterator itMsg;

    NetMessages m_SendMessages;

    typedef std::list<CDataBlock*>	 ListDataBlock;
    typedef std::list<CDataBlock*>::iterator itDB;
    ListDataBlock	m_ReadDataBlocks;	//�Ѿ���ȡ����������
    int	m_nReadDataSize;				//�Ѿ���ȡ���ݵĴ�С


    // ����˳���ȡ�ı���
    typedef std::map<int, CDataBlock*>	DataBlockMap;
    typedef std::map<int, CDataBlock*>::iterator itDBMap;
    ulong				m_ReadSequenceNumber;
    ulong				m_CurrentReadSequenceNumber;
    DataBlockMap		m_ReadBuffer;

public:
    bool IsLost()				{return m_bLost;}
    void SetLost(bool b)		{m_bLost = b;}
    bool IsQuit()				{return m_bQuit;}
    void SetQuit(bool b)		{m_bQuit = b;}

    void SetIsAccpet(bool b)	{m_bAccept = b;}
    bool IsAccept()				{return m_bAccept;}

    bool IsUsing()				{return m_bUsing;}
    void SetUsing(bool bUsing)	{m_bUsing=bUsing;}

    bool IsTransCong()			{return m_bTransfersStatus;}
    void SetTransCong(bool b)	{m_bTransfersStatus=b;}

    ushort GetUseCount()			{return m_nUseCount;}
    void IncUseCount();

    long GetServerType()                { return m_bServerType;}            //��ÿͻ��˷���������
    void SetServerType(long lservertype){m_bServerType = lservertype;}      //���ÿͻ��˷���������

    CServer*	GetServer()				{return m_pServers;	}
    virtual void SetSendRevBuf() {}

public:

    CServerClient(CServer*	pServers,CDataBlockAllocator* pDBAllocator);
    virtual ~CServerClient();

    void Init();
    void Release();
    void SetParam(bool bChecked,long lMaxMsgLen = 0xFFFFFFF,long lMaxBlockSendMsgNum=1024,
        long lMaxPendingWrBufNum=8192,long lMaxPendingRdBufNum=4096,
        long lMaxSendSizePerSecond=8192,long lMaxRcvSizePerSecond=4096,
        long lFlag=0);

    bool AddSendMsg(CBaseMessage* pMsg);
    bool AddReceiveData(long lSequeNumber,CDataBlock *pDB);
    //�õ���ǰ��Ϣ����
    long GetCurMsgLen();

    void PushReadDataBack(CDataBlock *pDB);
    CDataBlock *PopReadDataFront();
    void PushReadDataFront(CDataBlock *pDB);


    // ���ӳɹ�ʱ����
    virtual void OnAccept(int errorCode=0);
    // ��ɶ˿����ܵ���Ϣʱ����
    virtual	void OnReceive(int nErrorCode=0);
    // ��Է��Ͽ���ʱ�򼤻�
    virtual void OnClose(int errorCode=0);
    // ����Ϣ���͵�ʱ�򼤻�
    virtual void OnSend(int nErrorCode=0)	{};
    //����0�ֽ����ݷ���
    void OnSendZeroByteData();
    //�����紫��仯��ʱ��
    void OnTransferChange();

    virtual void OnOneMessageSizeOver(long lSize,long lPermitSize){};
    virtual void OnTotalMessageSizeOver(long lSize,long lPermitSize){};
    bool StartReadData();
    bool ReadFromCompletionPort(PER_IO_OPERATION_DATA* pIOData,CDataBlock *pDB=NULL);  //����ɶ˿ڶ�ȡ����
    virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);	// [TCP]��������

    virtual bool CheckMsg(CBaseMessage *pMsg)	{return true;}

    friend unsigned __stdcall NetThreadFunc(void* pArguments);
private:
    //��������������������������ʱ,����0�ֽ�����
    //��������һ������������⡢��������
    void SendZeroByteData();
    int Send(CBaseMessage *pMsg,int nFlags);
    //ͳ�ƿͻ��˵���������
private:
    //ͳ�ƽ���
    ulong	m_dwRcvStartTime;					//����ͳ�ƿ�ʼʱ��
    long	m_lCurTotalRcvSize;					//���ܰ����ܴ�С
    long	m_lCurRcvSizePerSecond;				//��ǰÿ����ܵ����ݴ�С
    long	m_lMaxRcvSizePerSecond;				//���ÿ����ܴ�С

    //ͳ�Ʒ���
    ulong	m_dwSendStartTime;					//����ͳ�ƿ�ʼʱ��
    long	m_lCurTotalSendSize;				//��ǰ�ܷ��ʹ�С
    long	m_lCurSendSizePerSecond;			//��ǰÿ�뷢�ʹ�С
    long	m_lMaxSendSizePerSecond;			//ÿ������ʹ�С

    bool	m_bChecked;							//�Ƿ������Ϣ
    long	m_lMaxMsgLen;						//�����Ϣ����
    long	m_lPendingWrBufNum;					//��ǰ�����ͻ��������ܴ�С
    long	m_lMaxPendingWrBufNum;				//�������ͻ��������ܴ�С
    long	m_lMaxPendingRdBufNum;				//����Ľ��ܻ��������ܴ�С
    ulong	m_lMaxBlockSendMsnNum;				//��������ķ�����Ϣ��

    long	m_lSendCounter;
    long	m_lRecvCounter;


    //�ϴ������ݳ���
    long	m_lLastMsgTotalSize;
    //�ϴ���Ϣ�ĳ���
    long	m_lLastMsgLen;
    //�ϴ���Ϣ������
    long	m_lLastMsgType;
    //�ϴ���Ϣ���ݿ�Ĵ���ǰ����
    long	m_lLastMsgPreDBNum;
    //�ϴ���Ϣ���ݿ�Ĵ�������
    long	m_lLastMsgPosDBNum;
    //�ϴ���Ϣ����ʱ���ĳ���
    long	m_lLastMsgRemainSize;
    //�ϴ���Ϣ�ƶ��ڴ��λ��
    long	m_lLastMsgMemMoveDBPos;
    //�ϴ��ƶ������
    long	m_lLastMsgMemMoveDBSize;

public:
    //���ӽ������ݴ�С
    long AddRecvSize(long lSize);
    //��ӷ������ݴ�С
    long AddSendSize(long lSize);
    //�õ���ǰÿ����ܴ�С
    long GetCurRecvSizePerSecond();
    long GetMaxRecvSizePerSecond(){return m_lMaxRcvSizePerSecond;}
    //�õ���ǰÿ�뷢�ʹ�С
    long GetCurSendSizePerSecond();

    long GetPendingWrBufNum()			{return m_lPendingWrBufNum;}
    void IncPendingWrBufNum(long lNum)	{m_lPendingWrBufNum += lNum;}
    void DecPendingWrBufNum(long lNum)	{m_lPendingWrBufNum -= lNum;}

    //����ȡ���кż�1
    void IncrReadSequenceNumber()	{m_ReadSequenceNumber = (m_ReadSequenceNumber+1)%MAXIMUMSEQUENSENUMBER;}
    ulong GetReadSequenceNumber()	{return m_ReadSequenceNumber;}
    void IncrCurReadSequenceNumber(){m_CurrentReadSequenceNumber = (m_CurrentReadSequenceNumber+1)%MAXIMUMSEQUENSENUMBER;}
    CDataBlock* GetNextReadDB();
};

