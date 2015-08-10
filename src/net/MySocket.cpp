//=============================================================================
/**
 *  file: MySocket.cpp
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

#include "MySocket.h"
#include "MsgDescribe.h"
#include "MsgCry.h"
#include <assert.h>


/// Ĭ������ͨ���������
long CMySocket::s_lEncryptType = 0;

//------------------------------------------------------------------
// ��ʼ��Winsock 2.0
//------------------------------------------------------------------
bool CMySocket::MySocketInit()
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 0);
	int nResult = WSAStartup(wVersionRequested, &wsaData);
	if (nResult != 0)
	{
		//PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"WSAStartup Failed!");
		return FALSE;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0)
	{
		//PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"Is not Winsock 2.0!");
		WSACleanup();
		return FALSE;
	}

	return TRUE;
}

//------------------------------------------------------------------
// �����˳�ʱ����
//------------------------------------------------------------------
void CMySocket::MySocketClearUp()
{
	WSACleanup();
}

/////////////////////////////////////////////////////////////////////////////
// CMySocket Construction

CMySocket::CMySocket()
{
	m_lProtocol = SOCK_STREAM;
	strcpy(m_strPeerIP,"");
	m_dwPeerPort = 0;
	m_dwPeerIP = 0;

	m_hSocket = INVALID_SOCKET;
			
	m_dwLastRecvPort = 0;		// [UDP] ��һ���յ�����Ϣ�Ķ˿�
	m_strLastRecvIP[0] = 0;		// [UDP] ��һ���յ�����Ϣ��IP

	m_bClose = false;
	m_bShutDown = false;
	m_lIndexID = 0;

	m_lFlag = 0;
}

CMySocket::~CMySocket()
{
	if( m_hSocket != INVALID_SOCKET )
	{
		int nRet = closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
}

//------------------------------------------------------------------
// ����SOCKET
//------------------------------------------------------------------
bool CMySocket::Create(UINT nSocketPort,			// �˿�
					   LPCTSTR lpszSocketAddress,	// ��ַ					   
					   int nSocketType,				// ����( TCP:SOCK_STREAM UDP:SOCK_DGRAM )
					   long lEvent,
					   bool bAsyncSelect  //defaultly,the flag==false,if create for client socket,set the flag=true
					   )
{
	m_lProtocol = nSocketType;
	m_hSocket = socket(PF_INET, nSocketType, 0);	// ��ʼ���׽���
	if( m_hSocket == INVALID_SOCKET )
	{
        //PutErrorString(NET_MODULE,"%-15s Create Socket Error = %d",__FUNCTION__,WSAGetLastError());
		return FALSE;
	}

	if( !Bind(nSocketPort, lpszSocketAddress) )	// �󶨶˿ں͵�ַ
	{
        //PutErrorString(NET_MODULE,"%-15s Create Socket Bind Error = %d",__FUNCTION__,WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}


//���� overlapped socket
bool CMySocket::CreateEx(UINT nSocketPort,LPCTSTR lpszSocketAddress,int nSocketType)
{
	m_lProtocol = nSocketType;
	//����һ�� overlapped socket
	m_hSocket = WSASocket(AF_INET,nSocketType,0,NULL,0,WSA_FLAG_OVERLAPPED);
		
	//��������
	if(m_hSocket == INVALID_SOCKET)
	{
        //PutErrorString(NET_MODULE,"%-15s CreateEx Socket Error = %d",__FUNCTION__,WSAGetLastError());
		return FALSE;
	}

	//��˿ں͵�ַ
	if(!Bind(nSocketPort, lpszSocketAddress))
	{
        //PutErrorString(NET_MODULE,"%-15s CreateEx Socket Bind Error = %d",__FUNCTION__,WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}


// �󶨶˿ں͵�ַ
bool CMySocket::Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress)
{
	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));

	LPSTR lpszAscii = (LPTSTR)lpszSocketAddress;
	sockAddr.sin_family = AF_INET;

	if (lpszAscii == NULL)
	{
		sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		uint32_t lResult = inet_addr(lpszAscii);
		if (lResult == INADDR_NONE)
		{
			WSASetLastError(WSAEINVAL);
			return FALSE;
		}
		sockAddr.sin_addr.s_addr = lResult;
	}

	sockAddr.sin_port = htons((u_short)nSocketPort);

	long hr = bind(m_hSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if(  hr == SOCKET_ERROR )
	{
        //PutErrorString(NET_MODULE,"%-15s Bind Error = %d",__FUNCTION__,WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}


//����socketѡ��
void CMySocket::SetSocketOpt()
{
	if( m_hSocket == INVALID_SOCKET)	return;
	//�ر�Socket��Nagle�㷨
	int bNodelay = 1; 
	int hr = setsockopt(m_hSocket,IPPROTO_TCP,TCP_NODELAY,(char *)&bNodelay,sizeof(bNodelay));//��������ʱ�㷨 
	if(  hr == SOCKET_ERROR )
	{
        //PutErrorString(NET_MODULE,"%-15s Disables the Nagle algorithm Error = %d",__FUNCTION__,WSAGetLastError());
	}
	return;
}


//�õ�������IP
void CMySocket::SetHostName()
{
	// ��ȡ����IP��ַ
	char strName[128];
	SOCKADDR_IN sockAddr;
	if(SOCKET_ERROR ==gethostname(strName, 128))
	{
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,WSAGetLastError());
		return;
	}
	LPHOSTENT lphost;
	lphost = gethostbyname(strName);
	if (lphost != NULL)
	{
		sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		SetPeerIP( sockAddr.sin_addr.S_un.S_addr );
		sprintf(strName, "%d.%d.%d.%d",
			sockAddr.sin_addr.S_un.S_un_b.s_b1,
			sockAddr.sin_addr.S_un.S_un_b.s_b2,
			sockAddr.sin_addr.S_un.S_un_b.s_b3,
			sockAddr.sin_addr.S_un.S_un_b.s_b4);

		SetPeerIP(strName);
	}
	else
	{
		assert(0);
	}
}

// ----------------------------------------------------
// �ر�Socket
// �ȵȴ�SocketThread�߳̽���
// �ú���������SocketThread�߳��е���
// ----------------------------------------------------
bool CMySocket::Close()
{
	if( m_hSocket != INVALID_SOCKET )
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
	m_bClose = true;
	return TRUE;
}

bool CMySocket::ShutDown()
{
	if( m_hSocket != INVALID_SOCKET )
	{
		if( shutdown(m_hSocket,SD_SEND)== SOCKET_ERROR)
		{
            //PutErrorString(NET_MODULE,"%-15sshutdown() Error = %d",__FUNCTION__,WSAGetLastError());
		}
	}
	m_bShutDown = true;
	return true;
}

// �ر�socket (�յ�FD_CLOSE)
void CMySocket::OnClose(int nErrorCode)
{
	CMySocket::Close();
} 

// [TCP] ��������
int CMySocket::Recv(void* lpBuf, int nBufLen, int nFlags)
{
	int rt = recv(m_hSocket, (LPSTR)lpBuf, nBufLen, nFlags);
	if( rt == SOCKET_ERROR )
	{
		long error = WSAGetLastError();
		if( error != WSAEWOULDBLOCK )
		{
            //PutErrorString(NET_MODULE,"%-15s ��ȡ����%d",__FUNCTION__,error);
		}
	}
	return rt;
}

// [TCP] ��������
int CMySocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	return true;
}


// [UDP] ��������
int CMySocket::RecvFrom(void* lpBuf, int nBufLen, char* strIP, uint32_t& dwPort, int nFlags )
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	int framlen = sizeof(addr);
	int rt = recvfrom(m_hSocket, (LPSTR)lpBuf, nBufLen, nFlags, (sockaddr*)&addr, &framlen);
	if( rt == SOCKET_ERROR )
	{
		long error = WSAGetLastError();
		if( error == WSAEWOULDBLOCK )
		{
			return 0;
		}
        //PutErrorString(NET_MODULE,"%-15s Socket RecvFrom Error = %d",__FUNCTION__,error);
		return FALSE;
	}

	strcpy(strIP, inet_ntoa(addr.sin_addr));
	dwPort = ntohs(addr.sin_port);
	return rt;
}

// [UDP] ��������
int CMySocket::Sendto(const void* lpBuf, int nBufLen, const char* strIP, uint32_t dwPort, int nFlags)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)dwPort);
	addr.sin_addr.S_un.S_addr = inet_addr(strIP);

_Begin:
	if( sendto(m_hSocket, (LPSTR)lpBuf, nBufLen, nFlags, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR )
	{
		long error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK)
		{
			goto _Begin;
		}
		return FALSE;
	}
	return TRUE;
}

// ��ȡ���׽��������ĵ�ַ
bool CMySocket::GetPeerName(char* rPeerAddress, UINT& rPeerPort)
{
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
    bool bResult = getpeername(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen) ? true : false;
	if (bResult)
	{
		rPeerPort = ntohs(sockAddr.sin_port);
		rPeerAddress = inet_ntoa(sockAddr.sin_addr);
	}
	return bResult;
}

// ��ȡ���ص�ַ
bool CMySocket::GetSockName(char* rSocketAddress, UINT& rSocketPort)
{
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
    bool bResult = getsockname(m_hSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen) ? true : false;
	if (bResult)
	{
		rSocketPort = ntohs(sockAddr.sin_port);
		rSocketAddress = inet_ntoa(sockAddr.sin_addr);
	}
	return bResult;
}

// ��ȡ���л��������ݵĴ�С
bool CMySocket::IOCtl(long lCommand, u_long* lpArgument)
{
	int rt = ioctlsocket(m_hSocket,lCommand,lpArgument);
	if (rt == SOCKET_ERROR)
	{
		long error = WSAGetLastError();
		return FALSE;
	}
	return TRUE;
}

long GetSocketID()
{
	static long lSocketID = 0;
	return ++lSocketID;
}

uint32_t STR2UL(const char *szIP)
{
	return inet_addr(szIP);
}

/// ������Կ(���CServerClient & CClients & �����˲���ʹ��)
void CMySocket::SetKey(int h, int l)
{
	*((uint32_t*)m_key) = h;
	*((uint32_t*)m_key+1) = l;

	::KeyInit(m_key, m_kn);
}
