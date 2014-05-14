//=============================================================================
/**
 *  file: ConClient.cpp
 *
 *  Brief:����ɶ˿�ʵ�ֵ��������Ӷˡ�
 *
 *  Authtor:wangqiao
 *	
 *	Datae:2007-6-11
 */
//=============================================================================

#include "ConClient.h"
#include "Servers.h"
#include "basemessage.h"
#include "io/Crc32Static.h"

using namespace std;


CConClient::CConClient(CServer*	pServers)
:m_pServers(pServers)
{
	assert(m_pServers);
	m_nBufferSize = MAX_MESSAGE_LENGTH;
	m_nIniMsgLength = MAX_MESSAGE_LENGTH;
	m_nSize = 0;
	m_pBuffer = new char[m_nBufferSize];

	m_lMaxSendBuffSize = MAX_MESSAGE_LENGTH;
	m_lSendBufferSize = MAX_MESSAGE_LENGTH;
	m_lCurSendBufferSize = 0;
	m_pSendBuffer = new char[m_lSendBufferSize];

	m_lIOOperatorNum = 0;
	m_bCloseFlag = false;
}

CConClient::~CConClient(void)
{
}

//����һ������ɶ˿ڰ��SOCKET
BOOL CConClient::CreateEx(	UINT nSocketPort,
							LPCTSTR lpszSocketAddress,
							int nSocketType)
{
	if(!CMySocket::CreateEx(nSocketPort,lpszSocketAddress,nSocketType))
	{
		PutDebugString("��CConClient::CreateEx(...)��,CMySocket::CreateEx()��������!");
		return FALSE;
	}
	//���÷��ͺͽ��ջ�����
	SetSendRevBuf();
	long lID = GetSocketID();
	SetIndexID(lID);
	return TRUE;
}

BOOL CConClient::Close()
{
	if(INVALID_SOCKET == m_hSocket)
		return FALSE;
	CancelIo((HANDLE)m_hSocket);
	CMySocket::Close();
	return TRUE;
}

//����ָ��������
//dwTimeOut==0��ʾ�첽���������0,��ʾͬ���ȴ�ʱ�䡣
BOOL CConClient::Connect(LPCTSTR lpszHostAddress, UINT nHostPort,DWORD dwTimeOut)
{
	if(m_hSocket == INVALID_SOCKET)	return FALSE;
	
	//�������ӵķ�����IP�Ͷ˿�
	SetPeerIP(lpszHostAddress);
	SetPeerPort(nHostPort);

	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	LPSTR lpszAscii = (LPTSTR)lpszHostAddress;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		PutDebugString("�ں���CConClient::Connect(...)��,��ַ��Ч��");
		return FALSE;
	}

	sockAddr.sin_port = htons((u_short)nHostPort);
	int ret = connect(m_hSocket,(SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if( ret == SOCKET_ERROR )
	{
		DWORD dwError = WSAGetLastError();
		if(  dwError != WSAEWOULDBLOCK )
		{
			char str[200];
			sprintf(str, "Clients Connect Socket Error = %d",dwError);
			PutDebugString(str);
			return FALSE;
		}

		//�Ⱥ�һ��ʱ��,���Ƿ��д���ж����ӳɹ����
		fd_set writefd;
		FD_ZERO(&writefd);
		FD_SET(m_hSocket,&writefd);
		timeval waittime = {dwTimeOut/1000,(dwTimeOut%1000)*1000};
		int nRet = select(0,NULL,&writefd,NULL,&waittime);
		if(nRet == 0 || nRet == SOCKET_ERROR)
		{
			dwError = WSAGetLastError();
			char str[200];
			sprintf(str, "Clients Connect Socket Error = %d",dwError);
			PutDebugString(str);
			return FALSE;
		}
	}
	return	TRUE;
}

bool CConClient::Read(PER_IO_OPERATION_DATA* pIOData)
{

	pIOData->DataBuf.len = pIOData->nBufSize;
	pIOData->DataBuf.buf = pIOData->pBuffer;
	pIOData->OperationType = SOT_CONReceive;
	DWORD dwFlag = 0;
	DWORD dwReceivByte;
	int nRet = WSARecv(m_hSocket,&(pIOData->DataBuf),1,&dwReceivByte,&dwFlag,&(pIOData->OverLapped),NULL);
	if(nRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			return false;
		}
	}
	return true;
}
//��������
int CConClient::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	if(m_lCurSendBufferSize <= 0)
	{
		return true;
	}

	PER_IO_OPERATION_DATA* pPerIOData = (PER_IO_OPERATION_DATA*)calloc(1,sizeof(PER_IO_OPERATION_DATA));
	//�����䲻���ڴ�ʱ
	if(pPerIOData == NULL)
	{
#ifdef _DEBUG
		char str[200] = "�������������Ϣʱ���ܷ����ڴ档";
		PutDebugString(str);
#endif // _DEBUG

		return false;
	}

	char* pTemptBuf = new char[m_lCurSendBufferSize];
	memcpy(pTemptBuf,m_pSendBuffer,m_lCurSendBufferSize);

	pPerIOData->OperationType = SOT_CONSend;
	pPerIOData->nBufSize = m_lCurSendBufferSize;
	pPerIOData->pBuffer= (CHAR*)pTemptBuf;
	pPerIOData->DataBuf.buf=pPerIOData->pBuffer;
	pPerIOData->DataBuf.len=pPerIOData->nBufSize;
	pPerIOData->hSocket = m_hSocket;

	DWORD dwSentNum = 0;
	int ret = WSASend(m_hSocket, &(pPerIOData->DataBuf), 1, &dwSentNum, nFlags,
		(OVERLAPPED*)pPerIOData, NULL);

	if (ret == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		if ( nError != WSA_IO_PENDING)
		{
			free(pPerIOData);
			char str[200];
			sprintf(str, "�������������Ϣ����(errorID:%d)��",nError);
			PutDebugString(str);
			return false;
		}
	}

	m_lCurSendBufferSize = 0;
	m_lIOOperatorNum++;
	//������ͻ��������ڹ涨����ԭ
	if(m_lSendBufferSize > m_lMaxSendBuffSize)
	{
		m_lSendBufferSize = m_lMaxSendBuffSize;
		SAFE_DELETE(m_pSendBuffer);
		m_pSendBuffer = new char[m_lSendBufferSize];

	}
	return true;
}
void CConClient::OnReceive(int nErrorCode)
{
	if(m_pServers==NULL)	return;
	long lPoint = 0;	// ��ǰ��Ч��������ַ
	while( m_nSize >= 12 )
	{
		long lRLESize = *((long*)(&m_pBuffer[lPoint]));
		//�Խ�����Ϣ��У��
		if(m_pServers->GetCheck())
		{
			//�Գ���������У��
			DWORD dwLenCrc32;
			CCrc32Static::DataCrc32(&lRLESize,4,dwLenCrc32);
			DWORD lClientSize = *((DWORD*)(&m_pBuffer[lPoint]+4));
			if(dwLenCrc32 != lClientSize)
			{
				char str[200];
				sprintf(str, "������(IP:%s)��������Ϣ����Ϣ����У��ʧ�ܡ�",GetPeerIP());
				PutDebugString(str);
				return;
			}
		}
		// ���������ݴ��ڵ���һ����Ϣ�ĳ���
		if( lRLESize <= m_nSize )
		{
			//����Ϣ��������У��
			if(m_pServers->GetCheckMsgCon())
			{
				DWORD dwConCrc32;
				CCrc32Static::DataCrc32(&m_pBuffer[lPoint]+12,lRLESize-12,dwConCrc32);
				DWORD lClientConCrc32= *((DWORD*)(&m_pBuffer[lPoint]+8));
				//��⵽���ݲ�һ��
				if(dwConCrc32 != lClientConCrc32)
				{
					m_pServers->AddForbidIP(GetDwordIP());
					m_pServers->QuitClientBySocketID(GetIndexID());
					char str[200];
					sprintf(str, "��������(IP:%s)��������Ϣ����Ϣ����У��ʧ�ܡ�",GetPeerIP());
					PutDebugString(str);
					return;
				}
			}

			CBaseMessage* pMsg = CreateMsg(&m_pBuffer[lPoint]+12, lRLESize-12);
			// ��Ϣ�����ɹ���ѹ����Ϣ����
			if( pMsg )
			{
				pMsg->SetClientSocketID(GetIndexID());
				m_pServers->GetRecvMessages()->PushMessage( pMsg );
				m_nSize -= lRLESize;
				lPoint += lRLESize;		// �ƶ�������ָ��

				// ����ǳ�������Ϣ���ѻ�������С��������
				if( m_nBufferSize > m_nIniMsgLength && m_nSize <= m_nIniMsgLength )
				{
					char *pTemp = new char[m_nSize];
					memcpy(pTemp, &m_pBuffer[lPoint], m_nSize);

					SAFE_DELETE(m_pBuffer);
					m_nBufferSize = MAX_MESSAGE_LENGTH;
					m_pBuffer = new char[m_nBufferSize];

					memcpy(m_pBuffer, pTemp, m_nSize);
					SAFE_DELETE(pTemp);
					lPoint = 0;
				}
			}
			else
			{
				// ����������ǰ�յ�����������
				m_nSize = 0;
				char str[256];
				sprintf(str, "������(IP:%s)��������Ϣ����CreateMessage error",GetPeerIP());
				PutDebugString(str);
				return;
			}
		}
		else
		{
			// ����������ǰ�յ�����������
			if( lRLESize < 0 )
			{
				m_nSize = 0;
				char str[256];
				sprintf(str, "������(IP:%s)��������Ϣ����������Ϣ(RleSize=%d)��������ǰ�յ�����������",
							GetIP(),lRLESize);
				PutDebugString(str);
				return;
			}
			// �յ���Ϣ���������Ϣ�����������µĻ�����
			if( lRLESize > m_nBufferSize )
			{
				char *pTemp = new char[m_nSize];
				memcpy(pTemp, &m_pBuffer[lPoint], m_nSize);
				SAFE_DELETE(m_pBuffer);
				m_nBufferSize = lRLESize;
				m_pBuffer = new char[m_nBufferSize];
				memcpy(m_pBuffer, pTemp, m_nSize);
				SAFE_DELETE(pTemp);
				return;
			}
			break;
		}
	}
	// �Ѳ���ȫ����Ϣ�ƶ���������ͷ��
	if( lPoint > 0 )
	{
		memmove(m_pBuffer, &m_pBuffer[lPoint], m_nSize);
		lPoint = 0;
	}
}
void CConClient::OnConnect(int nErrorCode)
{
}
void CConClient::OnClose(int nErrorCode)
{
}
void CConClient::OnSend(int nErrorCode)
{
}

// ��������ʹ�õĽӿ�
void CConClient::HandleReceive()
{
}
void CConClient::HandleConnect()
{
}
void CConClient::HandleClose()
{
}

void CConClient::AddReceiveData(CHAR* pBuf,int nBufSize)
{
	if(pBuf == NULL || nBufSize <= 0)
		return;

	if(m_nSize + nBufSize <= m_nBufferSize)
	{
		memcpy(&m_pBuffer[m_nSize],pBuf,nBufSize);
		m_nSize = m_nSize+nBufSize;
	}
	else
	{
		CHAR* pNewBuf = new CHAR[m_nSize+nBufSize];
		memcpy(&pNewBuf[0],&m_pBuffer[0],m_nSize);
		memcpy(&pNewBuf[m_nSize],pBuf,nBufSize);

		delete []m_pBuffer;
		m_pBuffer = pNewBuf;
		m_nBufferSize = m_nSize+nBufSize;
		m_nSize = m_nBufferSize;
	}
}
bool CConClient::AddSendData(void* pBuf,int nBuffSize)
{
	if(pBuf == NULL)	return false;
	//�Ѿ���ʾ�˹ر�
	if( GetCloseFlag() == true)
		return true;

	if(m_lCurSendBufferSize + nBuffSize > m_pServers->GetMaxClientSendBufSize() )
	{
	 	return false;
	}

	if(m_lCurSendBufferSize + nBuffSize <= m_lSendBufferSize)
	{
		memcpy(m_pSendBuffer+m_lCurSendBufferSize,pBuf,nBuffSize);
	}
	else
	{
		char* pTemptBuf = new char[m_lCurSendBufferSize];
		memcpy(pTemptBuf,m_pSendBuffer,m_lCurSendBufferSize);

		SAFE_DELETE(m_pSendBuffer);
		m_lSendBufferSize = (m_lCurSendBufferSize+nBuffSize)*2;
		m_pSendBuffer = new char[m_lSendBufferSize];

		memcpy(m_pSendBuffer,pTemptBuf,m_lCurSendBufferSize);
		memcpy(m_pSendBuffer+m_lCurSendBufferSize,pBuf,nBuffSize);

		SAFE_DELETE(pTemptBuf);
	}
	m_lCurSendBufferSize = m_lCurSendBufferSize+nBuffSize;
	return true;
}

CBaseMessage* CConClient::CreateMsg(void* pBuf, unsigned long nSize)
{
	return CBaseMessage::CreateWithoutRLE(pBuf,nSize);
}
