#include "MemClient.h"
#include "BaseMessage.h"

bool g_bRun = true;

CMemClient::CMemClient(void)
{
}

CMemClient::~CMemClient(void)
{
}

bool CMemClient::Initial(const char* szMapObjName,
						 CDataBlockAllocator* pDBAllocator)
{
	m_pMapFile = new CMapFile;

	static bool bFlag = false;
	if(!bFlag)
	{
		m_pMapFile->SetDBAllocator(pDBAllocator);
		bFlag = true;
	}

	if( !m_pMapFile->Reset(FMT_Client, szMapObjName) )
	{
		DELETE_SAFE(m_pMapFile);
		return false;
	}
	
	unsigned uID;
	m_hProcThread = (HANDLE)_beginthreadex(NULL, 0, ProcFunc, this, 0, &uID);
	m_hSendMsgThread = (HANDLE)_beginthreadex(NULL, 0, SendMsgFunc, this, 0, &uID);

	if (NULL == m_hProcThread ||
		NULL == m_hSendMsgThread)
	{
		PutDebugString(NET_MODULE,"�ڴ���Ϣ�շ��̴߳���ʧ��. ErrorID: %lu.", GetLastError());
		return false;
	}

	return true;
}

bool CMemClient::Release(void)
{
	g_bRun = false;
	if (m_pMapFile != NULL)
	{
		m_pMapFile->Exit();
	}

	WaitForSingleObject(m_hSendMsgThread, INFINITE);
	CloseHandle(m_hSendMsgThread);
	WaitForSingleObject(m_hProcThread, INFINITE);
	CloseHandle(m_hProcThread);
	DELETE_SAFE(m_pMapFile);

	long lCount = m_RecvMsgs.GetSize();
	while( lCount-- > 0 )
	{
		CBaseMessage* pMsg = m_RecvMsgs.PopMessage();
		if( pMsg != NULL )
		{
			CBaseMessage::FreeMsg(pMsg);
		}
	}

	return true;
}

bool CMemClient::Send(CBaseMessage* pMsg,
					  eFileMapServerType eType)
{
	bool bRet = false;
	if( m_pMapFile )
	{
		pMsg->SetNetFlag(eType);
		m_pMapFile->SendMsg(pMsg);
		bRet = true;
	}

	return bRet;
}


bool CMemClient::SyncSend(CBaseMessage* pMsg,
		eFileMapServerType eType )
{
	bool bRet = false;
	if( m_pMapFile )
	{
		pMsg->SetNetFlag(eType);
		m_pMapFile->SyncSendMsg(pMsg);
		bRet = true;
	}
	return bRet;
}

bool CMemClient::Recv(void)
{
	CBaseMessage* pMsg = m_pMapFile->RecvMsg();
	if( pMsg != NULL)
	{
		m_RecvMsgs.PushMessage(pMsg);
	}

	return true;
}

// �����߳�ID������Ϣ
void CMemClient::Send(void)
{
	// ���������ʱ���˴�ΪNULL
	if (m_pMapFile != NULL)
	{
		while (true)
		{
			long lSendNum = m_pMapFile->SendMsg();
			if (lSendNum == -1)		// �߳��˳�
			{
				break;
			}
		}
	}
}

unsigned __stdcall ProcFunc(void* pArguments)
{	
	CMemClient* pClient = (CMemClient*)pArguments;
	if( NULL == pClient )
	{
		_endthreadex( 0 );
		return 0;
	}

#ifndef _DEBUG
	__try
	{
#endif
		while( g_bRun )
		{
			pClient->Recv();
		}

#ifndef _DEBUG
	}
	__except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode(),"ProcFunc"))
	{
		PutDebugString(NET_MODULE,"********�����������߳�ProcFunc������鿴���µĴ��󱨸��ļ�********");
	}
#endif

	_endthreadex( 0 );
	return 0;
}

unsigned __stdcall SendMsgFunc(void* pArguments)
{	
	CMemClient* pClient = (CMemClient*)pArguments;
	if( NULL == pClient )
	{
		_endthreadex( 0 );
		return 0;
	}

#ifndef _DEBUG
	__try
	{
#endif
		pClient->Send();

#ifndef _DEBUG
	}
	__except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode(),"SendMsgFunc"))
	{
		PutDebugString(NET_MODULE,"********�����������߳�SendMsgFunc������鿴���µĴ��󱨸��ļ�********");
	}
#endif

	_endthreadex( 0 );
	return 0;
}