//=============================================================================
/**
 *  file: Clients.h
 *
 *  Brief:��Ҫ�ǿͻ���ʹ�õ������װ�����¼�����ʵ�ֵ�����ӿ�
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
#include "mysocket.h"
#include "MsgQueue.h"
#include "SocketCommands.h"

const int CONNECT_TIMEOUT = 10000;		// ���ӵȴ�ʱ��

class CMessage;
class CDataBlockAllocator;

class CClient : public CMySocket  
{
protected:
	CDataBlockAllocator* m_pDBAllocator;

	typedef std::list<tagSocketOper*>	SocketOpers;
    typedef std::list<tagSocketOper*>::iterator itSockOP;

	uint32_t m_nMaxFreeSockOperNum;
	SocketOpers	m_FreeSocketOpers;
	CRITICAL_SECTION	m_CSSockOper;


    typedef std::list<CBaseMessage*>	 NetMessages;
    typedef std::list<CBaseMessage*>::iterator itMsg;
	NetMessages m_SendMessages;

    typedef std::list<CDataBlock*>	 ListDataBlock;
    typedef std::list<CDataBlock*>::iterator itDB;
	ListDataBlock	m_ReadDataBlocks;	//�Ѿ���ȡ����������
	int	m_nReadDataSize;				//�Ѿ���ȡ���ݵĴ�С


	HANDLE m_hConnectEventSuccess;		// ���ӳɹ�
	bool m_bConnect;					// �Ƿ�����

public:
	void PushReadDataBack(CDataBlock *pDB);
	CDataBlock *PopReadDataFront();
	void PushReadDataFront(CDataBlock *pDB);
	
	HANDLE m_hSocketEvent[2];	// 0 ������Ϣ 1 ����SocketThread
	HANDLE m_hSocketThread;		// �����¼��Ľ���
	HANDLE m_hNetClientThread;	// �ͻ����������߳�

	bool CreateSocketThread();	// ���������̹߳���socket
	bool ExitSocketThread();	//�Ƴ�ClientSocket���߳�

	tagSocketOper* AllocSockOper();
	void FreeSockOper(tagSocketOper* pSockOper);
	
	friend unsigned __stdcall SocketThread(void* pArguments);
	friend void DoSocketThread(CClient* pClient);
	friend unsigned __stdcall NetClientThreadFunc(void* pArguments);		//�ͻ����������߳� 
	friend void DoNetClientThreadFunc(CClient* pClient);					//�ͻ����������߳� 

	bool m_bSendData;					//��ʾ�ͻ����Ƿ���Է�����
protected:
	bool m_bControlSend;							//���Ʒ��͵�һ��ȫ�ֱ���

	CSocketCommands		m_SocketOperaCommands;		//������������������
	CMsgQueue			m_RecvMessages;				//���յ�����Ϣ����

public:
	void	SetControlSend(bool bSend)	{m_bControlSend=bSend;}
	bool	GetControlSend()			{return m_bControlSend;}
	
	CSocketCommands& GetSocketCommand()	{return m_SocketOperaCommands;}
	CMsgQueue*	GetRecvMessages()	{return & m_RecvMessages;}

	bool Start(CDataBlockAllocator* pDBAllocator,long nMaxFreeSockOperNum,long lFlag);
	void Exit();

	// ����һ�����¼����ư��SOCKET
	virtual bool Create(UINT nSocketPort = 0,
		LPCTSTR lpszSocketAddress = NULL,				
		int nSocketType = SOCK_STREAM,	// TCP:SOCK_STREAM UDP:SOCK_DGRAM
		long lEvent = FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE,
		bool bAsyncSelect=true  //asyncselect flag,if create socket for client,set the flag=true
		);
	bool AsyncSelect(long lEvent = FD_READ|FD_WRITE|FD_ACCEPT|FD_CONNECT|FD_CLOSE);	// ���������¼�����Ӧ

	int ASend(CBaseMessage *pMsg, int nFlags = 0);
	bool AddSendMsg(CBaseMessage* pMsg);
	int Send();
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);	// [TCP]��������

	virtual bool Close();				// �ر�socket
	virtual void OnReceive(int nErrorCode);
	void OnReceive(CDataBlock *pDB,long lReadNum);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	//�õ���ǰ��Ϣ����
	long GetCurMsgLen();
	//������Ϣ
	void DecordMsg();


	// ��������ʹ�õĽӿ�
	virtual void HandleReceive(){};
	virtual void HandleConnect() {};
	virtual void HandleClose() {};

	bool Connect(LPCTSTR lpszHostAddress, UINT nHostPort);	// ����
	bool ConnectServer(LPCTSTR lpszHostAddress, UINT nHostPort);//����
	bool IsConnect();		// �Ƿ�����
	long WaitForConnect();	// �ȴ�����

	CClient();
	virtual ~CClient();

//ͳ������
protected:
	bool				m_bCheck;					//�Ƿ���

	uint32_t				m_dwSendStartTime;			//ͳ�ƿ�ʼʱ��
	long				m_lTotalSendSize;			//���͵�������
	long				m_lSendSizePerSecond;		//���뷢�͵�����
	LONG64				m_llSendSize;

	uint32_t				m_dwRecvStartTime;			//ͳ�ƿ�ʼʱ��
	long				m_lTotalRecvSize;			//����������
	long				m_lRecvSizePerSecond;		//ÿ���������
	LONG64				m_llRecvSize;
public:

	bool	GetCheck()	{ return m_bCheck; }

	long	AddSendSize(long lSize);
	long	GetSendSizePerSecond()			{return m_lSendSizePerSecond;}
	LONG64  GetTotalSendSize()				{return m_llSendSize;}

	long	AddRecvSize(long lSize);
	long	GetRecvSizePerSecond()			{return m_lRecvSizePerSecond;}
	LONG64	GetTotalRecvSize()				{return m_llRecvSize;}
};

