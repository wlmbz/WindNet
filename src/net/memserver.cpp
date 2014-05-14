
#include "MemServer.h"
#include <process.h>
#include "BaseMessage.h"
#include "base/utils.h"

struct ArgList
{
	ArgList()
		: _svr(NULL)
		, _id(0)
	{}

	~ArgList()
	{
		_svr = NULL;
		_id = 0;
	}

	CMemServer * _svr;
	DWORD _id;
};

bool g_bRunS = true;

CMemServer::CMemServer(void)
: m_pDBAllocator(NULL)
{
}

CMemServer::~CMemServer(void)
{
}

bool CMemServer::Initial(CDataBlockAllocator* pDBAllocator)
{
	m_pDBAllocator = pDBAllocator;

	return true;
}

bool CMemServer::Release(void)
{
	g_bRunS = false;
	ITR_MF_CONF itr = m_MapFiles.begin();
	CMapFile *pMapFile = NULL;
	for( ; itr != m_MapFiles.end(); ++itr )
	{
		pMapFile = itr->second.pMapFile;
		if (pMapFile != NULL)
		{
			pMapFile->Exit();
		}
		WaitForSingleObject(itr->second.hSendMsgThread, INFINITE);
		CloseHandle(itr->second.hSendMsgThread);
		WaitForSingleObject(itr->second.hProcThread, INFINITE);
		CloseHandle(itr->second.hProcThread);
		DELETE_SAFE(pMapFile);
	}

	return true;
}

// ���SERVER, �����߳�ID.
unsigned CMemServer::AddServer(DWORD dwID,
							   const char* szMapObj,
							   const char* szMapFile,
							   const char* szOutputDir)
{
	tagConf tConf;
	tConf.dwID = dwID;
	strcpy(tConf.szMapObj, szMapObj);
	strcpy(tConf.szMapFile, szMapFile);
	strcpy(tConf.szOutputDir, szOutputDir);
	tConf.pMapFile = new CMapFile;
	tConf.pMapFile->SetDBAllocator(m_pDBAllocator);
	if( !tConf.pMapFile->Reset(FMT_Server, szMapObj, szMapFile) )
	{
		DELETE_SAFE(tConf.pMapFile);
		return 0;
	}

	tConf.pMapFile->SetCurDir(szOutputDir);
	unsigned uID(0);

	{
		ArgList *pArgList = new ArgList;
		pArgList->_svr = this;
		pArgList->_id = dwID;
		tConf.hProcThread = (HANDLE)_beginthreadex(NULL, 0, ProcFuncS, pArgList, 0, &uID);
	}
	{
		ArgList *pArgList = new ArgList;
		pArgList->_svr = this;
		pArgList->_id = dwID;
		tConf.hSendMsgThread = (HANDLE)_beginthreadex(NULL, 0, SendMsgFuncS, pArgList, 0, &uID);
	}
	if( NULL != tConf.hProcThread &&
		NULL != tConf.hSendMsgThread )
	{
		m_MapFiles[tConf.dwID] = tConf;
	}
	else
	{
		//PutDebugString(NET_MODULE, "�ڴ���Ϣ�շ��̴߳���ʧ��, ErrorID: %lu.", GetLastError());
	}

	return dwID;
}

void CMemServer::Reset(DWORD dwID)
{
	CMapFile* pMapFile = NULL;
	ITR_MF_CONF itr = m_MapFiles.find(dwID);
	if( itr != m_MapFiles.end() &&
		(pMapFile = itr->second.pMapFile) != NULL )
	{
		pMapFile->Reset();
	}
}

void CMemServer::ResetAll(void)
{
	CMapFile* pMapFile = NULL;
	ITR_MF_CONF itr = m_MapFiles.begin();
	for( ; itr != m_MapFiles.end(); ++itr )
	{
		if( (pMapFile = itr->second.pMapFile) != NULL )
		{
			pMapFile->Reset();
		}
	}
}

bool CMemServer::Send(DWORD dwID,
					  CBaseMessage* pMsg,
					  eFileMapServerType eType)
{
	bool bRet = false;
	CMapFile* pMapFile = NULL;
	ITR_MF_CONF itr = m_MapFiles.find(dwID);
	if( itr != m_MapFiles.end() &&
		(pMapFile = itr->second.pMapFile) != NULL )
	{
		pMsg->SetNetFlag(eType);
		pMapFile->SendMsg(pMsg);
		bRet = true;
	}
	return bRet;
}

bool CMemServer::Run(DWORD dwID)
{
	CMapFile* pMapFile = NULL;
	ITR_MF_CONF itr = m_MapFiles.find(dwID);
	if( m_MapFiles.end() == itr ||
		NULL == (pMapFile = itr->second.pMapFile) )
	{
		return false;
	}

	// ��ȡ��Ϣ
	CBaseMessage* pMsg = pMapFile->RecvMsg();
	if( pMsg != NULL)
	{
		switch(	pMsg->GetType() )
		{
		case MSG_MEM_OPEN_FILE:
			{
				char filename[MAX_FILENAME_LEN];
				pMsg->GetStr(filename, MAX_FILENAME_LEN);
				OpenFile(dwID, filename);
				CBaseMessage::FreeMsg(pMsg);
			}
			break;

		case MSG_MEM_WRITE_FILE:
			{
				char filename[MAX_FILENAME_LEN];
				char filecontent[MAX_CONTENT_LEN];
				pMsg->GetStr(filename, MAX_FILENAME_LEN);
				long SmInfoOrder = pMsg->GetLong();
				sprintf(filecontent, "%d ", SmInfoOrder);
				if( (*itr).second.lTestID != -1 && SmInfoOrder != 0)
				{
					if( (SmInfoOrder -(*itr).second.lTestID) != 1 )
					{
						static int lNo = 0;
						if (++lNo > 100000)
						{
							//PutDebugString(NET_MODULE, "������Ϣ����lastID:%d,curID:%d,�������%d.", (*itr).second.lTestID,SmInfoOrder,lNo);
							lNo = 0;
						}
					}
				}
				(*itr).second.lTestID = SmInfoOrder;

				pMsg->GetStr(&(filecontent[strlen(filecontent)]), MAX_CONTENT_LEN);
				WriteFile(dwID, filename, filecontent);
				CBaseMessage::FreeMsg(pMsg);
			}
			break;

		case MSG_MEM_CLEAR_FILE:
			{
				char filename[MAX_FILENAME_LEN];
				pMsg->GetStr(filename, MAX_FILENAME_LEN);
				ClearFile(dwID, filename);
				CBaseMessage::FreeMsg(pMsg);
			}
			break;

		case MSG_MEM_CLOSE_FILE:
			{
				char filename[MAX_FILENAME_LEN];
				pMsg->GetStr(filename, MAX_FILENAME_LEN);
				CloseFile(dwID, filename);
				CBaseMessage::FreeMsg(pMsg);
			}
			break;

		default:
			{
				pMsg->SetNetFlag(dwID);
				m_RecvMsgs.PushMessage(pMsg);
			}
			break;
		}
	}
	else if( 0 == m_RecvMsgs.GetSize() )
	{
		Sleep(1);
	}

	pMapFile->ClosePassiveFiles();

	return true;
}

// �����߳�ID������Ϣ
void CMemServer::Send(DWORD dwID)
{
	// ���������ʱ���˴�ΪNULL
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL)
	{
		while (true)
		{
			long lSendNum = pMapFile->SendMsg();
			if (lSendNum == -1)		// �߳��˳�
			{
				break;
			}
		}
	}
}

// �����ļ����õ���Ӧ�ļ����
tagFileInfo0x* CMemServer::GetFile(DWORD dwID, const char* pszFileName)
{
	tagFileInfo0x* pFileInfo = NULL;
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL)
	{
		pFileInfo = pMapFile->GetFile(pszFileName);
	}
	return pFileInfo;
}

//���һ���ļ�����
tagFileInfo0x* CMemServer::AddFile(DWORD dwID, const char* pszFileName)
{
	tagFileInfo0x* pFileInfo = NULL;
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL &&
		pszFileName != NULL)
	{
		pFileInfo = pMapFile->AddFile(pszFileName);
	}
	return pFileInfo;
}

//���ļ�
void CMemServer::OpenFile(DWORD dwID, const char* pszFileName)
{
	if (pszFileName == NULL ||
		strlen(pszFileName) < 4)
	{
		//PutDebugString(NET_MODULE,"���ļ�(%s)����, �ļ����Ƿ�.",pszFileName);
		return;
	}

	// �ȹرմ��ڵ��ļ����
	CloseFile(dwID, pszFileName);
	// ���һ���ļ�����
	AddFile(dwID, pszFileName);
}

//�ر��ļ�
void CMemServer::CloseFile(DWORD dwID, const char* pszFileName)
{
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL)
	{
		pMapFile->CloseFile(pszFileName);
	}
}

// ����ļ�
void CMemServer::ClearFile(DWORD dwID, const char* pszFileName)
{
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL)
	{
		pMapFile->ClearFile(pszFileName);
	}
}

// д�ļ�
void CMemServer::WriteFile(DWORD dwID, const char* pszFileName, const char* pszContent)
{
	CMapFile* pMapFile = GetMapFile(dwID);
	if (pMapFile != NULL)
	{
		pMapFile->WriteFile(pszFileName, pszContent);
	}
}

// �����߳�ID��ȡMapFile����
CMapFile* CMemServer::GetMapFile(DWORD dwID)
{
	CMapFile* pMapFile = NULL;
	ITR_MF_CONF itr = m_MapFiles.find(dwID);
	if( m_MapFiles.end() != itr )
	{
		pMapFile = itr->second.pMapFile;
	}

	if (NULL == pMapFile)
	{
		//PutDebugString(NET_MODULE, "����MapFile(%u)����, ErrorID:%lu.", dwID, GetLastError());
	}

	return pMapFile;
}

unsigned __stdcall ProcFuncS(void* pArguments)
{
	Sleep(1000);
	ArgList *pArgList = (ArgList*)pArguments;
	CMemServer* pServer = pArgList->_svr;
	if( NULL == pServer )
	{
		_endthreadex( 0 );
		return 0;
	}

#ifndef _DEBUG
	__try
	{
#endif
		DWORD dwID = pArgList->_id;
		delete pArgList;
		while( g_bRunS )
		{
			pServer->Run(dwID);
		}

#ifndef _DEBUG
	}
	__except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
	{
		PutDebugString(NET_MODULE,"********�����������̳߳�����鿴���µĴ��󱨸��ļ�********");
	}
#endif

	_endthreadex( 0 );
	return 0;
}

unsigned __stdcall SendMsgFuncS(void* pArguments)
{
	Sleep(1000);
	ArgList *pArgList = (ArgList*)pArguments;
	CMemServer* pServer = pArgList->_svr;
	if( NULL == pServer )
	{
		_endthreadex( 0 );
		return 0;
	}

#ifndef _DEBUG
	__try
	{
#endif
		DWORD dwID = pArgList->_id;
		delete pArgList;
		pServer->Send(dwID);

#ifndef _DEBUG
	}
	__except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
	{
		PutDebugString(NET_MODULE,"********�����������̳߳�����鿴���µĴ��󱨸��ļ�********");
	}
#endif

	_endthreadex( 0 );
	return 0;
}