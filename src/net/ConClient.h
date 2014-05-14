//=============================================================================
/**
 *  file: ConClient.h
 *
 *  Brief:����ɶ˿�ʵ�ֵ��������Ӷˡ�
 *
 *  Authtor:wangqiao
 *	
 *	Datae:2007-6-11
 */
//=============================================================================

#pragma once



#include "mysocket.h"

class CServer;

class CConClient :
	public CMySocket
{
protected:
	char *m_pBuffer;					// ���ܻ���
	int m_nIniMsgLength;				// ��ʼ����Ϣ����
	int m_nBufferSize;					// ��������С
	int m_nSize;						// �������е�ǰ���ݴ�С

	char *m_pSendBuffer;				// ���ͻ�����
	long m_lMaxSendBuffSize;			// ����ͻ�����
	long m_lSendBufferSize;				// ���ͻ�������С
	long m_lCurSendBufferSize;			// ��ǰ��������С
	long m_lSendSize;					// ÿ�η��͵����ݴ�С
public:
	CConClient(CServer*	pServers);
public:
	virtual ~CConClient(void);

private:
	CServer*		m_pServers;			//���Ƶĸ��׽���

	long	m_lIOOperatorNum;			//��ǰ�ص����͵ĸ���
	bool	m_bCloseFlag;				//�Ƿ���ɾ�����
public:
	//����һ������ɶ˿ڰ��SOCKET
	virtual bool CreateEx(UINT nSocketPort = 0,
		LPCTSTR lpszSocketAddress = NULL,				
		int nSocketType = SOCK_STREAM	// TCP:SOCK_STREAM UDP:SOCK_DGRAM
		);
	virtual bool Close();

	//����ָ��������
	bool Connect(LPCTSTR lpszHostAddress, UINT nHostPort,ulong dwTimeOut=0);

	//��������
	bool Read(PER_IO_OPERATION_DATA* pIOData);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual void OnReceive(int nErrorCode=0);
	virtual void OnConnect(int nErrorCode=0);
	virtual void OnClose(int nErrorCode=0);
	virtual void OnSend(int nErrorCode=0);

	// ��������ʹ�õĽӿ�
	virtual void HandleReceive();
	virtual void HandleConnect();
	virtual void HandleClose();

	void AddReceiveData(CHAR* pBuf,int nBufSize);
	bool AddSendData(void* pBuf,int nBuffSize);

	long GetIOOperatorNum()	{return m_lIOOperatorNum;}
	void DecIoOperatorNum()	{m_lIOOperatorNum--;}

	void	SetCloseFlag(bool b)		{m_bCloseFlag = b;}
	bool	GetCloseFlag()				{return m_bCloseFlag;}

	long	GetCurSendBuffSize()		{return m_lCurSendBufferSize;}
	long	GetSendBuffSize()			{return m_lSendBufferSize;}

	CBaseMessage* CreateMsg(void* pBuf, unsigned long nSize);
};
