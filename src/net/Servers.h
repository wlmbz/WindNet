//=============================================================================
/**
 *  file: Servers.h
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


#pragma once

#include <map>
#include <list>
#include <vector>
#include "MySocket.h"
#include "MsgQueue.h"
#include "SocketCommands.h"
#include "IOCompletionPort.h"
#include "ServerWorker.h"

#define CP_TIMEOUT	200

class CServerClient;
class CBaseMessage;
class CDataBlockAllocator;
const int MAX_CLIENTS = 2048;


class CServer : public CMySocket
{
private:
	
public:
	//�������������
	bool	Start(	byte bMode,CDataBlockAllocator* pDBAllocator,
					long nMaxFreeSockOperNum = 100,long nMaxFreeIOOperNum=100,long lIOOperDataBufNum = 2,
					bool bCheck=false,long lForbidTime=1000,
					long lMaxMsgLen=102400,long lMaxConNums=1000,long lTotalClientsNum=10,
					long lPendingWrBufNum=4096,long lPendingRdBufNum=2048,
					long lMaxSendSizePerSecond=8192,long lMaxRecvSizePerSecond =4096,
					long lMaxBlockSendMsgNum = 1024,
					long lConPendingWrBufNum=10240,long lConPendingRdBufNum=10240,
					long lConMaxSendSizePerSecond=81920,long lConMaxRecvSizePerSecond=81920,
					long lConMaxBlockSendMsgNum=1024);

	void InitClients();
	void ReleaseClients();

	//�˳�������׽���
	void Exit();
	//�˳���ɶ˿ڹ������߳�
	void ExitWorkerThread(void);

	tagSocketOper* AllocSockOper();
	void FreeSockOper(tagSocketOper* pSockOper);

	PER_IO_OPERATION_DATA* AllocIoOper();
	void FreeIoOper(PER_IO_OPERATION_DATA* pPerIOData);

	//����һ���ͻ���
	CServerClient*	AllocClient(bool bAccept);
	void FreeClient(CServerClient* pClient);
	// ����һ���ͻ���
	CServerClient* FindClient(long lSocketID);
	// ɾ��һ���ͻ���
	bool FreeClient(long lSocketID);

    CIOCompletionPort&  GetIOCompletionPort() { return completion_port_; }
private:
    byte	m_bMode;	//����ģʽ

    typedef std::list<tagSocketOper*>	SocketOpers;
    typedef std::list<tagSocketOper*>::iterator itSockOP;

    uint32_t m_nMaxFreeSockOperNum;
    SocketOpers	m_FreeSocketOpers;
    CRITICAL_SECTION	m_CSSockOper;

    typedef std::list<PER_IO_OPERATION_DATA*>	ListIOOpers;
    typedef std::list<PER_IO_OPERATION_DATA*>::iterator itIOOper;

    uint32_t m_nMaxFreeIOOperNum;
    ListIOOpers	m_FreeListIOOpers;
    CRITICAL_SECTION	m_CSIOOper;

    //���Ŀ������ӿͻ�������
    long	m_lMaxFreeConClientNum;

    CRITICAL_SECTION	m_CSClient;
    CServerClient	**m_pClients;
    long	m_lTotalClientsNum;


    CDataBlockAllocator*	m_pDBAllocator;

    //ÿ��IO������DataBufNum����
    long	m_lIOOperDataBufNum;

	// �����˷��������׽�������
	long m_lCurClientCount;

	typedef std::map<long,CServerClient*> AcceptClients;
    typedef std::map<long, CServerClient*>::iterator itAClient;
	//�������ܵ��׽���
	AcceptClients	m_AClients;

	//Ԥ�����AccClients
    typedef std::list<CServerClient*> FreeAccClients;
    typedef std::list<CServerClient*>::iterator itFAClient;
	//����Ԥ����AccClients����
	long	m_nMaxFreeAccClientNum;
	FreeAccClients	m_FreeAClients;


    std::vector<ServerWorkerPtr> m_hWorkerThreads;	//�������߳̾������
	HANDLE m_hNetMainTheads;					//�������������߳�
	HANDLE m_hAcceptThread;						//acceptThread

	bool m_bAcceptThreadExit;					//�Ƿ����AcceptThread
	bool m_bWorkerThreadsExit;					//�Ƿ�����������߳�

    CIOCompletionPort completion_port_;

public:
	long GetClientCount()	{ return m_lCurClientCount;}

	HANDLE GetAcceptThread()	{ return m_hAcceptThread; }
	// ����Server���׽��֣��������״̬���ȴ��ͻ��˵�����
	bool Host(UINT dwPort, const char* strIP,long lFlag,uint32_t nSocketType=SOCK_STREAM);
	bool Listen(int nConnectionBacklog=SOMAXCONN);			// listen


	//�첽������Ϣ
	int ASend(long lIndexID,CBaseMessage *pMsg);	
		
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnAccept(int nErrorCode); 

	bool  DisConn(long lIndexID);
	bool  DisConnAll();
	//�ر�����Socket
	bool  CloseAll();
	//��ʼ������
	bool OnClientInit(CServerClient* pClient);
	// error��ʱ������
	void OnError(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,int errorCode);
	void OnClose(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID);
	void OnReceive(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lReadNum);
	void OnSend(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lSendNum);
	void OnSendZeorByte(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID);
	void OnDisConn(long lIndexID);
	long OnDisConnAll();
	long OnCloseAll();

	bool CreateNetMainThread();					//�����������߳�
	bool CreateWorkerThreads(int nProcNum);		//��������ɶ˿��ϵȴ��Ĺ������߳�
	bool CreateAcceptThread();					//�������տͻ�Socket���߳�
	bool AssociateSocketWithCP(SOCKET socket,uint32_t dwCompletionKey);	//���ڰһ��SOCKET����ɶ˿�

	CServer();
	virtual ~CServer();

	friend unsigned __stdcall NetThreadFunc(void* pArguments);
	friend void DoNetThreadFunc(CServer* pServer);
	friend unsigned __stdcall AcceptThreadFunc(void* pArguments);

    CSocketCommands&    GetSocketOperCommands() { return m_SocketOperaCommands; }
//
private:
	CSocketCommands		m_SocketOperaCommands;		//������������������
	CMsgQueue			m_RecvMessages;				//���յ�����Ϣ����

public:
	CMsgQueue* GetRecvMessages()	{return &m_RecvMessages;}

//ͳ�ƿͻ��˵���������
protected:
	bool				m_bCheck;					//�Ƿ���ͻ���
	long				m_lForbidTime;				//����IP��ʱ��
	long				m_lMaxMessageLength;		//��Ϣ������󳤶�

	long				m_lMaxClientConNum;			//���Ŀͻ�����������
	long				m_lPendingWrBufNum;			//�ȴ������д��������
	long				m_lPendingRdBufNum;			//�ȴ���������������
	long				m_lMaxSendSizePerSecond;	//���Ƶ�ÿ���ӷ��ʹ�С
	long				m_lMaxRecvSizePerSecond;	//���Ƶ�ÿ���ӽ��ܴ�С 
	long				m_lMaxBlockSendMsgNum;		//����������͵���Ϣ����

	//������Ӷ˵�ͳ��
	long				m_lConPendingWrBufNum;		//���Ӷ˵ȴ������д��������
	long				m_lConPendingRdBufNum;		//���Ӷ˵ȴ���������������	
	long				m_lConMaxSendSizePerSecond;	//���Ӷ�����ÿ���ӷ������ݵĴ�С
	long				m_lConMaxRecvSizePerSecond;	//���Ӷ�����ÿ���ӽ������ݵĴ�С
	long				m_lConMaxBlockSendMsgNum;	//���Ӷ�����������͵���Ϣ����


	//��Ը������ӵ�����ͳ��
	struct tagDataStat
	{
		uint32_t dwSendStartTime;			//ͳ�ƿ�ʼʱ��
		long lTotalSendSize;			//���͵�������
		uint32_t dwRecvStartTime;			//ͳ�ƿ�ʼʱ��
		long lTotalRecvSize;			//����������
	};

    typedef std::map<long, tagDataStat>	mapNetDataStat;
    typedef std::map<long, tagDataStat>::iterator itDataStat;
	mapNetDataStat	m_NetDataStats;


	//������Ϣ����ͳ��
	struct tagMsgStat
	{
		long	lNum;//���ͻ���յ������Ǹ�
		long	lTotalSize;//�ܴ�С
	};

    typedef std::map<long, tagMsgStat>	mapMsgStat;
    typedef std::map<long, tagMsgStat>::iterator itMsgStat;
	//������Ϣ��ͳ��
	mapMsgStat	m_SendMsgsStat;
	//������Ϣ��ͳ��
	mapMsgStat	m_RecvMsgsStat;

    typedef std::map<long, long>	mapMaxMsgLen;
    typedef std::map<long, long>::iterator itMaxMsgLen;
	mapMaxMsgLen	m_SendMsgMaxLen;
	mapMaxMsgLen	m_RecvMsgMaxLen;

	CRITICAL_SECTION	m_CSMsgStat;

	
    std::map<u_long, uint32_t>	m_ForbidIPs;				//����IP�б�
	uint32_t				m_lMaxBlockConnetNum;		//����������������
	uint32_t				m_lSendInterTime;			//���������Ժ󵽽��ܵ����ݵ�������¼�

	uint32_t				m_dwWorkThreadTick;			//��ɶ˿ڹ����߳���������
	uint32_t				m_dwNetThreadTick;			//�����߳���������
	uint32_t				m_dwAcceptThreadTick;		//���������߳���������
	

    std::map<uint32_t, uint32_t>	g_NewAcceptSockets;
public:

	inline void IncWorkThreadTick()	{m_dwWorkThreadTick++;}
	inline void IncNetThreadTick()	{m_dwNetThreadTick++;}
	inline void IncAcceptThreadTick()	{m_dwAcceptThreadTick++;}

	uint32_t	GetWorkThreadTick()	{return m_dwWorkThreadTick;}
	uint32_t	GetNetThreadTick()	{return m_dwNetThreadTick;}
	uint32_t	GetAcceptThreadTick() {return m_dwAcceptThreadTick;}

	bool	GetCheck()	{ return m_bCheck; }
	long	GetMaxMsgLen()		{ return m_lMaxMessageLength; }

	void	AddForbidIP(u_long laddr);
	bool	FindForbidIP(u_long laddr);

	long	AddSendSize(long lFlagID,long lSize);
	long	GetSendSizePerSecond(long lFlagID);

	long	AddRecvSize(long lFlagID,long lSize);
	long	GetRecvSizePerSecond(long lFlagID);

	//��ӷ�����Ϣ����ͳ��
	void	AddSendMsgStat(long lMsgType,long lSize);
	//��ӽ�����Ϣ����ͳ��
	void	AddRecvMsgStat(long lMsgType,long lSize);
	//�����Ϣͳ����Ϣ��net.log�ļ�
	void	OutputMsgStatInfo(uint32_t lIntelTime =1800000);

	long	GetMaxConNum()				{return m_lMaxClientConNum;}
	long	GetPendingWrBufNum()		{return m_lPendingWrBufNum;}
	long	GetPendingRdBufNum()		{return m_lPendingRdBufNum;}

	long	GetConPendingWrBufNum()		{return m_lConPendingWrBufNum;}
	long	GetConPendingRdBufNum()		{return m_lConPendingRdBufNum;}

	void	SetParamEx(long lBlockConNum,long lSendInterTime)
	{m_lMaxBlockConnetNum=lBlockConNum;m_lSendInterTime=lSendInterTime;}

	void	AddNewAcceptSocket(uint32_t dwSocketID);
	void	RemoveNewAcceptSocket(uint32_t dwSocketID);
	void	DoNewAcceptSocket();


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//�������Ӷ�
private:
	
public:	
	//�������ӷ�����
	long Connect(LPCTSTR lpszHostAddress, UINT nHostPort,long lFlag,uint32_t dwTimeOut=0);
	//��������
	long Connect(CServerClient* pConClient,LPCTSTR lpszHostAddress, UINT nHostPort,uint32_t dwTimeOut=0);
	//�����ر�����
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

};
