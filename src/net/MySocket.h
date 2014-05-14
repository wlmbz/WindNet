//=============================================================================
/**
 *  file: MySocket.h
 *
 *  Brief:������࣬�ṩ����������ӿ�
 *
 *  Authtor:wangqiao
 *	
 *	Datae:2005-4-15
 *
 *	Modify:2007-4-13,�����˴���ͽṹ���Ż���Ч��
 */
//=============================================================================

#ifndef _MYSOCKET_H_
#define _MYSOCKET_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <iostream>
#include <vector>
#include "winsock2.h"

using namespace std;

const int MAX_MESSAGE_LENGTH = 1048576*2;
class CBaseMessage;
class CDataBlock;

//���׽��ֵĲ�������
enum SocketOperType
{	
	SOT_Send,			//����
	SOT_Receive,		//����
	SOT_Accept,			//����
	SOT_SendZeroByte,	//����0�ֽ�����

	SOT_CONSend,		//���Ӷ˷���
	SOT_CONReceive,		//���Ӷ˽���
};

//IO�����ṹ:��С:44BYTE
typedef class PER_IO_OPERATION_DATA
{
public:
	OVERLAPPED		OverLapped;
	LPWSABUF		pDataBuf;
	long			m_lDataBufNum;
	void*			pParam;
	SOCKET			hSocket;			// �������׽���
	SocketOperType	OperationType;

	int				m_nSequenceNumber;	//�������к�

public:
	PER_IO_OPERATION_DATA(long lWSABUFNum)
	:pParam(NULL)
	,hSocket(INVALID_SOCKET)
	,m_nSequenceNumber(0)
	,m_lDataBufNum(lWSABUFNum)
	{
		pDataBuf = new WSABUF[m_lDataBufNum];
	}

	~PER_IO_OPERATION_DATA()
	{
		if(pDataBuf)
			delete []pDataBuf;
	}

	void ResetDataBufSize(long lWSABUFNum)
	{
		if(lWSABUFNum != m_lDataBufNum)
		{
			if(pDataBuf)
				delete []pDataBuf;
			m_lDataBufNum = lWSABUFNum;
			pDataBuf = new WSABUF[m_lDataBufNum];
		}
	}

}*LPER_IO_OPERATION_DATA;

/*-----------------------------------------------------------
 [CMySocket] WinSock����
 ���಻ֱ��ʹ�ã�һ��������ʹ�á�
 
 ���ܣ�
	* ����Socket
	* �󶨶˿ں͵�ַ
	* ���������¼�����Ӧ
	* ��������
	* ��������
	* �ر�socket
------------------------------------------------------------*/
class CMessage;
class CMySocket
{
public:
	static bool MySocketInit();		// ��������ʱ���ã���ʼ��Winsock
	static void MySocketClearUp();	// �����˳�ʱ����

	CMySocket();
	virtual ~CMySocket();

//////////////////////////////////////////////////////////////////////////
// ������Ϣ
//////////////////////////////////////////////////////////////////////////
protected:
	long m_lProtocol;		// ��ǰЭ��	TCP:SOCK_STREAM UDP:SOCK_DGRAM
	long m_lIndexID;		// ����Socket�����ID

	//char m_strIP[32];		// socket IP
	//ulong m_dwIP;			// IP(4byte)
	//ulong m_dwPort;			// socket�˿�

	char m_strPeerIP[32];	// �Է���IP
	ulong m_dwPeerIP;		// IP(4byte)
	ulong m_dwPeerPort;		// �Է��Ķ˿ں�

	bool m_bClose;			// socket�Ƿ�ر�
	bool m_bShutDown;		// �Ƿ�shutdown

	long m_lFlag;			// ���߼������õı�־
	uchar m_key[8];							/// ��Կ��
	uchar m_kn[16][6];						/// ��Կ��
	static long s_lEncryptType;						/// ��Ҫͨ�ż��ܵ�����

public:
	void GetKn(uchar kn[16][6])  { memcpy(m_kn, kn, 16*6); }

	/// ������Կ(���C/S���Բ�ͬ)
	void SetKey(int h, int l);
	
	void SetIndexID(long lID)	{m_lIndexID=lID;}
	long GetIndexID()			{return m_lIndexID;	}

	long GetProtocol()			{return m_lProtocol;}
	void SetProtocol(long l)	{m_lProtocol=l;}

	bool IsClose()				{return m_bClose;}
	void SetClose(bool b)		{m_bClose = b;}

	bool IsShutDown()			{return m_bShutDown;}
	void SetShutDown(bool b)	{m_bShutDown = b;}
	void SetPeerIP(const char* str)	{ strcpy(m_strPeerIP, str); }
	char* GetPeerIP(void)		{ return m_strPeerIP; }
	void SetPeerIP(ulong dw)	{ m_dwPeerIP = dw; }
	ulong GetDWPeerIP(void)		{ return m_dwPeerIP; }
	void SetPeerPort(ulong dw)	{ m_dwPeerPort = dw;}
	ulong GetPeerPort(void)		{ return m_dwPeerPort; }

	void SetFlag(long lFlag)	{m_lFlag = lFlag;}
	long GetFlag()				{return m_lFlag;}
    bool IsEncryptType(long lFlag) { return (s_lEncryptType & lFlag) ? true : false; }
	static void SetEncryptFlag(long l) { s_lEncryptType = l; }

//////////////////////////////////////////////////////////////////////////
// [UDP] ������һ����Ϣ����Ϣ
//////////////////////////////////////////////////////////////////////////
protected:
	ulong m_dwLastRecvPort;		// [UDP] ��һ���յ�����Ϣ�Ķ˿�
	char m_strLastRecvIP[32];	// [UDP] ��һ���յ�����Ϣ��IP

public:
	void SetLastRecvPort(ulong w)		{m_dwLastRecvPort = w;}
	ulong GetLastRecvPort(void)			{return m_dwLastRecvPort;}
	void SetLastRecvIP(const char* str)	{strcpy(m_strLastRecvIP,str);}
	char* GetLastRecvIP()				{return m_strLastRecvIP;}

//////////////////////////////////////////////////////////////////////////
// �ӿ�
//////////////////////////////////////////////////////////////////////////
public:
	// ����һ�����¼����ư��SOCKET
	virtual bool Create(UINT nSocketPort = 0,
				LPCTSTR lpszSocketAddress = NULL,				
				int nSocketType = SOCK_STREAM,	// TCP:SOCK_STREAM UDP:SOCK_DGRAM
				long lEvent = FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE,
				bool bAsyncSelect=true);
	
	//����һ������ɶ˿ڰ��SOCKET
	virtual bool CreateEx(	UINT nSocketPort,
					LPCTSTR lpszSocketAddress,
					int nSocketType=SOCK_STREAM);

	bool Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress = NULL);	// �󶨶˿ں͵�ַ

	//����socketѡ��
	virtual void SetSocketOpt();

	//�õ�������IP
	void SetHostName();
	
	virtual bool Close();	// �ر�socket���ȵȴ�SocketThread����
	//���ŵĹر��׽���
	bool ShutDown();
	
	virtual int Recv(void* lpBuf, int nBufLen, int nFlags = 0);			// [TCP]��������
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);	// [TCP]��������
	virtual int RecvFrom(void* lpBuf, int nBufLen, char* strIP, ulong& dwPort, int nFlags = 0);		// [UDP]��������
	virtual int Sendto(const void* lpBuf, int nBufLen, const char* strIP, ulong dwPort, int nFlags = 0);	// [UDP]��������

public:
	virtual void OnReceive(int nErrorCode)	{};
	virtual void OnSend(int nErrorCode)		{};
	virtual void OnOutOfBandData(int nErrorCode)	{};
	virtual void OnAccept(int nErrorCode)	{};
	virtual void OnConnect(int nErrorCode)	{};
	virtual void OnClose(int nErrorCode=0);	// �ر�socket (�յ�FD_CLOSE)
	virtual void SetSendRevBuf(){}

//////////////////////////////////////////////////////////////////////////
// ���ߺ���
//////////////////////////////////////////////////////////////////////////
public:
	bool GetPeerName(char * rPeerAddress, UINT& rPeerPort);		// ��ȡ���׽��������ĵ�ַ
	bool GetSockName(char * rSocketAddress, UINT& rSocketPort);	// ��ȡ���ص�ַ
	bool IOCtl(long lCommand=FIONREAD, ulong* lpArgument=0);	// ��ȡ���л��������ݵĴ�С
	
//////////////////////////////////////////////////////////////////////////
// SocketThread
//////////////////////////////////////////////////////////////////////////
public:
	SOCKET m_hSocket;			// �׽���
	friend class	CMessage;
};

//�õ�һ��ID�����Socket
long GetSocketID();
ulong STR2UL(const char *szIP);

#endif
