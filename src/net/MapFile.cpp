//=============================================================================
/**
*  file: MapFile.cpp
*
*  Brief:ͨ��Windows���ļ�Ӱ����ƣ���װ�˼򵥵�Ӱ���ļ���������
*			������ص��ļ���־��¼
*
*  Authtor:wangqiao
*	
*	Datae:2008-10-29
*/
//=============================================================================


#include "MapFile.h"
#include <Sddl.h>
#include <mmsystem.h>
#include <tchar.h>
#include <assert.h>
#include "BaseMessage.h"
#include "common/SafeDelete.h"


using namespace std;


const int MAX_BLOCK_MSGS = 200;
const int ERROR_REPORT_FRQ = 2;
const int MAX_ERROR_REPORT = 100;

CMapFile::CMapFile(void)
:m_hFile(INVALID_HANDLE_VALUE)
,m_hMapObject(NULL)
,m_pszMapViewS(NULL)
,m_pszMapViewC(NULL)
,m_hSendEvent(NULL)
,m_hMutexS(NULL)
,m_hEventS(NULL)
,m_hMutexC(NULL)
,m_hEventC(NULL)
,m_eType(FMT_No)
,m_bExit(false)
{
}

CMapFile::~CMapFile(void)
{
	Release();
}

void CMapFile::SetDBAllocator(CDataBlockAllocator* pDBA)
{
	m_pDBAllocator = pDBA;
}

//��ʼ��
bool CMapFile::Initial(eFileMapServerType eFMType,const char* pszMapObjName,const char* pFileName)
{
	if(!pszMapObjName)	return false;
	m_eType = (eFileMapServerType)(m_eType|eFMType);
	SECURITY_ATTRIBUTES sa;

	// m_hMapObject
	if (m_eType&FMT_Server)	// ��Ϊ������
	{
		if (pFileName)
		{
			m_hFile = CreateFile(_TEXT(pFileName),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,
				OPEN_ALWAYS,0,0);
			if(INVALID_HANDLE_VALUE == m_hFile)
			{
				//PutDebugString(NET_MODULE,"��ʼ��MapFileʱ, �����ļ�(%s)����. ErrorID: %lu.",pFileName,GetLastError());
				m_eType = FMT_No;
				return false;
			}
		}

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = FALSE;
		TCHAR* szSecDes = TEXT("D:")		// �Զ�����ʿ��Ʊ�
			TEXT("(D;OICI;GA;;;BG)")		// ��ֹ�ڽ�guests����
			TEXT("(D;OICI;GA;;;AN)")		// ��ֹ������¼����
			TEXT("(A;OICI;GRGWGX;;;AU)")	// ������֤�˺Ŷ�|д|ִ��
			TEXT("(A;OICI;GA;;;BA)");		// �������Ա�˺���ȫ����
		ConvertStringSecurityDescriptorToSecurityDescriptorA(szSecDes,
			SDDL_REVISION_1,
			&sa.lpSecurityDescriptor,
			NULL);

		m_hMapObject = CreateFileMapping(m_hFile, &sa,
			PAGE_READWRITE, 0, MAX_MAP_VIEW_SIZE * 2, _TEXT(pszMapObjName));
	}
	else if (m_eType&FMT_Client)	// ��Ϊ�ͻ���
	{
		m_hMapObject = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,_TEXT(pszMapObjName));
	}
	if (!m_hMapObject) 
	{ 
		if (m_eType&FMT_Server) 
		{ 
			CloseHandle(m_hFile); 
		}
		m_eType = FMT_No;
		//PutDebugString(NET_MODULE,"��ʼ��MapFileʱ, ����Ӱ���ļ�(%s)����. ErrorID: %lu.",pszMapObjName,GetLastError());
		return false;
	}

	// m_pszMapViewS|C
	m_pszMapViewS = MapViewOfFile(m_hMapObject, SECTION_ALL_ACCESS, 0, 0, MAX_MAP_VIEW_SIZE);
	m_pszMapViewC = MapViewOfFile(m_hMapObject, SECTION_ALL_ACCESS, 0, MAX_MAP_VIEW_SIZE, 0);
	if (!m_pszMapViewS || !m_pszMapViewC)
	{
		CloseHandle(m_hMapObject); 
		if (m_eType&FMT_Server) 
		{
			UnmapViewOfFile(m_pszMapViewS);
			CloseHandle(m_hFile); 
		}
		m_eType = FMT_No;
        //PutDebugString(NET_MODULE,"��ʼ��MapFileʱ, Ӱ������ʱ(%sC|S)����. ErrorID: %lu.",pszMapObjName,GetLastError());
		return false;
	}

	// m_hMutexS|C
	char pszMutexObjS[256] = "";
	char pszMutexObjC[256] = "";
	_snprintf(pszMutexObjS,256, "%s_MutexS", pszMapObjName);
	_snprintf(pszMutexObjC,256, "%s_MutexC", pszMapObjName);
	if (m_eType&FMT_Server) 
	{ 
		m_hMutexS = CreateMutex(&sa,FALSE,pszMutexObjS);
		m_hMutexC = CreateMutex(&sa,FALSE,pszMutexObjC);
	}
	else if (m_eType&FMT_Client)
	{
		m_hMutexS = OpenMutex(MUTEX_ALL_ACCESS, FALSE, pszMutexObjS);
		m_hMutexC = OpenMutex(MUTEX_ALL_ACCESS, FALSE, pszMutexObjC);
	}
	if (!m_hMutexS || !m_hMutexC)
	{
		if (m_eType&FMT_Server) 
		{
			CloseHandle(m_hMutexS);
			UnmapViewOfFile(m_pszMapViewS);
			UnmapViewOfFile(m_pszMapViewC);
			CloseHandle(m_hFile); 
		}
		CloseHandle(m_hMapObject); 
		m_eType = FMT_No;
		//PutDebugString(NET_MODULE,
		//	"��ʼ��MapFileʱ, ����|�򿪻������(%s|C)����. ErrorID: %lu.",
		//	pszMutexObjS,
		//	GetLastError());
		return false;
	}

	// m_hEventS|C
	char pszEventObjS[256] = "";
	char pszEventObjC[256] = "";
	char pszEventObjSend[256] = "";
	_snprintf(pszEventObjS, 256, "%s_EventS", pszMapObjName);
	_snprintf(pszEventObjC, 256, "%s_EventC", pszMapObjName);
	if (m_eType&FMT_Server) 
	{ 
		m_hEventS = CreateEvent(&sa, FALSE, FALSE, pszEventObjS);
		m_hEventC = CreateEvent(&sa, FALSE, FALSE, pszEventObjC);
		_snprintf(pszEventObjSend, 256, "%s_SendEventS", pszMapObjName);
	}
	else if (m_eType&FMT_Client)
	{
		m_hEventS = OpenEvent(EVENT_ALL_ACCESS, FALSE, pszEventObjS);
		m_hEventC = OpenEvent(EVENT_ALL_ACCESS, FALSE, pszEventObjC);
		_snprintf(pszEventObjSend, 256, "%s_SendEventC", pszMapObjName);
	}
	if (!m_hEventS || !m_hEventC)
	{
		if (m_eType&FMT_Server) 
		{ 
			UnmapViewOfFile(m_pszMapViewS);
			UnmapViewOfFile(m_pszMapViewC);
			CloseHandle(m_hFile); 
		}
		CloseHandle(m_hMapObject); 
		m_eType = FMT_No;
		//PutDebugString(NET_MODULE,
		//	"��ʼ��MapFileʱ, ����|���¼�����(%s S|C|Send)����. ErrorID: %lu.",
		//	pszMapObjName,
		//	GetLastError());
		return false;
	}

	m_hSendEvent = CreateEvent(NULL, FALSE, FALSE, pszEventObjSend);
	if (!m_hSendEvent)
	{
		if (m_eType&FMT_Server) 
		{ 
			UnmapViewOfFile(m_pszMapViewS);
			UnmapViewOfFile(m_pszMapViewC);
			CloseHandle(m_hFile); 
		}
		CloseHandle(m_hMapObject); 
		m_eType = FMT_No;
		//PutDebugString(NET_MODULE,
		//	"��ʼ��MapFileʱ, �����¼�����(%s)����. ErrorID: %lu.",
		////	pszEventObjSend,
		//	GetLastError());
		return false;
	}

	Reset();
	return true;
}

// �ͷ���Դ
void CMapFile::Release()
{
	ReleaseAllFiles();
	if (m_eType == FMT_Server)
	{
		UnmapViewOfFile(m_pszMapViewC);
		UnmapViewOfFile(m_pszMapViewS);
		CloseHandle(m_hFile);
	}
	CloseHandle(m_hMapObject);
}

void CMapFile::Exit(void)
{
	HANDLE hEvent = NULL;
	if (m_eType&FMT_Server)
	{
		hEvent = m_hEventC;
	}
	else if (m_eType&FMT_Client)
	{
		hEvent = m_hEventS;
	}

	SendExitMsg();		// ������SetEvent(hEvent)֮ǰ
	SetEvent(hEvent);
}

// ��������
bool CMapFile::Reset(eFileMapServerType eFMType,const char* pszMapObjName,const char* pFileName)
{
	// ���ͷ�
	Release();
	return Initial(eFMType, pszMapObjName, pFileName);
}

void CMapFile::Reset(void)
{
	if (m_eType&FMT_Server)			// S����ʼ����д���ݵ�λ��ֵ
	{
		WaitForSingleObject(m_hMutexS, INFINITE);
		//ǰ4���ֽڱ�ʾ��ʼд���ݵ�λ��
		*(long*)(m_pszMapViewS) = 8;
		// �κ�4���ֽڱ�ʾ�����ݵ�λ��
		*(long*)((char*)m_pszMapViewS+4) = 8;
		ReleaseMutex(m_hMutexS);

		WaitForSingleObject(m_hMutexC, INFINITE);
		//ǰ4���ֽڱ�ʾ��ʼд���ݵ�λ��
		*(long*)(m_pszMapViewC) = 8;
		// �κ�4���ֽڱ�ʾ�����ݵ�λ��
		*(long*)((char*)m_pszMapViewC+4) = 8;
		ReleaseMutex(m_hMutexC);
	}
}

// �ͷ������ļ�
void CMapFile::ReleaseAllFiles(void)
{
	ITR_MAP_FILE it = m_Files.begin();
	tagFileInfo0x *pFileInfo = NULL;
	for (; it != m_Files.end(); ++it)
	{
		pFileInfo = it->second;
		if (pFileInfo != NULL)
		{
			if (pFileInfo->pFile != NULL)
			{
				fclose(pFileInfo->pFile);
				pFileInfo->pFile = NULL;
			}
			SAFE_DELETE(pFileInfo);
		}
	}

	m_Files.clear();
}

//���ռ��С�Ƿ�����
bool CMapFile::CheckViewSpace(long lNeedSize)
{
	LPVOID pMapView = NULL;
	if (m_eType&FMT_Server)
	{
		pMapView = m_pszMapViewS;
	}
	else if (m_eType&FMT_Client)
	{
		pMapView = m_pszMapViewC;
	}

	//д���ݵ�λ��
	long lWritePos = *(long*)(pMapView);
	//������λ��
	long lReadPos = *(long*)((char*)pMapView + 4);
	//������С
	if( lNeedSize > (MAX_MAP_VIEW_SIZE-sizeof(8)) )
		return false;
	//��������
	if( lWritePos < lReadPos && (lWritePos+lNeedSize) >= lReadPos)
		return false;
	//��������
	if( (lWritePos >=  lReadPos) && lNeedSize > (MAX_MAP_VIEW_SIZE-lWritePos+lReadPos-8) )
		return false;
	return true;
}

//�����´�д���ݵ�λ��
int CMapFile::WriteData(int nPos,const char *pData,int nLen)
{
	char* pPos = NULL;
	if (m_eType&FMT_Server)
	{
		pPos = (char*)m_pszMapViewS + nPos;
	}
	else if (m_eType&FMT_Client)
	{
		pPos = (char*)m_pszMapViewC + nPos;
	}

	if( (nPos+nLen) < MAX_MAP_VIEW_SIZE )
	{
		memcpy(pPos,pData,nLen);
		return nPos+nLen;
	}
	int nSize = MAX_MAP_VIEW_SIZE-nPos;
	memcpy(pPos,pData,nSize);
	//��ͷ��ʼ
	if (m_eType&FMT_Server)
	{
		pPos = (char*)m_pszMapViewS + 8;
	}
	else if (m_eType&FMT_Client)
	{
		pPos = (char*)m_pszMapViewC + 8;
	}
	else
	{
		//PutDebugString(NET_MODULE, "�ڴ�ӳ������������ô���(%d). ErrorID: %lu.", m_eType, GetLastError());
		assert(0);
	}

	memcpy(pPos,pData+nSize,nLen-nSize);
	return 8+nLen-nSize;
}

//��Χ�´ζ����ݵ�λ��
int CMapFile::ReadData(int nPos,char *pData,int nLen)
{
	char* pPos = NULL;
	if (m_eType&FMT_Server)
	{
		pPos = (char*)m_pszMapViewC + nPos;
	}
	else if (m_eType&FMT_Client)
	{
		pPos = (char*)m_pszMapViewS + nPos;
	}
	else
	{
		//PutDebugString(NET_MODULE, "�ڴ�ӳ������������ô���(%d). ErrorID: %lu.", m_eType, GetLastError());
		assert(0);
	}

	if( (nPos+nLen) < MAX_MAP_VIEW_SIZE )
	{
		memcpy(pData,pPos,nLen);
		return nPos+nLen;
	}

	int nSize = MAX_MAP_VIEW_SIZE-nPos;
	memcpy(pData,pPos,nSize);
	//��ͷ��ʼ
	if (m_eType&FMT_Server)
	{
		pPos = (char*)m_pszMapViewC + 8;
	}
	else if (m_eType&FMT_Client)
	{
		pPos = (char*)m_pszMapViewS + 8;
	}
	memcpy(pData+nSize,pPos,nLen-nSize);
	return 8+nLen-nSize;
}

// �����߳��˳�
void CMapFile::SendExitMsg(void)
{
	m_bExit = true;
	SetEvent(m_hSendEvent);
}

// д��Ϣ(�߳�ͳһ����)
long CMapFile::SendMsg(void)
{
	msgQueue TemptMsgQueue;
	WaitForSingleObject(m_hSendEvent, INFINITE);
	if (m_bExit)
	{
		//PutDebugString(NET_MODULE,"�����߳��˳�. ErrorID: %lu.", GetLastError());
		return -1;
	}

	m_SendMsgs.GetAllMessage(TemptMsgQueue);
	CDataBlock* pDB = NULL;
	
	long lNum = 0;
	
	bool bDelaySend = false;
	while (!TemptMsgQueue.empty())
	{		
		CBaseMessage* pMsg = TemptMsgQueue.front();
		if( pMsg )
		{
			eRetSendMsg eRet = SendMsgimmediately(pMsg);
			if( eRet == SendMsg_Ok)
			{
				lNum++;
			}
			else if(eRet == SendMsg_NoSpace)
			{
				bDelaySend = true;
				break;
			}
			TemptMsgQueue.pop_front();
			CBaseMessage::FreeMsg(pMsg);
		}
		else
		{
			//PutDebugString(NET_MODULE,"��������ϢΪNULL. ErrorID: %lu.", GetLastError());
		}
	}
	if(bDelaySend)
	{
		m_SendMsgs.PushMsgsoFront(TemptMsgQueue);
	}

	TemptMsgQueue.clear();
	return lNum;
}


//��ⷢ��һ����Ϣ
CMapFile::eRetSendMsg CMapFile::SendMsgimmediately(CBaseMessage* pMsg)
{
	eRetSendMsg eRet = SendMsg_Error;
	if(!pMsg)	return eRet;

	HANDLE hMutex = NULL;
	HANDLE hEvent = NULL;
	LPVOID pMapView = NULL;
	if (m_eType&FMT_Server)
	{
		pMapView = m_pszMapViewS;
		hMutex = m_hMutexS;
		hEvent = m_hEventS;
	}
	else if (m_eType&FMT_Client)
	{
		pMapView = m_pszMapViewC;
		hMutex = m_hMutexC;
		hEvent = m_hEventC;
	}
	else
	{
		//PutDebugString(NET_MODULE, "�ڴ�ӳ������������ô���(%d). ErrorID: %lu.", m_eType, GetLastError());
		assert(0);
	}

	CDataBlock* pDB = NULL;
	long lDataSize = 0;
	vector<CDataBlock*>& DataBlocks = pMsg->GetMsgData();
	long lBlockSize = (long)DataBlocks.size();
	for( long i = 0; i < lBlockSize; ++i )
	{
		pDB = DataBlocks[i];
		lDataSize += pDB->GetCurSize();
	}
	long lNeedSize = lDataSize + sizeof(long);

	if (lNeedSize > 10e6)
	{
		static int lNo = 0;
		if (++lNo % ERROR_REPORT_FRQ == 1 && lNo < MAX_ERROR_REPORT)
		{
			//PutDebugString(NET_MODULE, "�����ڴ���Ϣ����: %ld, �������: %d. ErrorID: %lu.", lNeedSize, lNo, GetLastError());
		}
		return eRet;
	}

	WaitForSingleObject(hMutex, INFINITE);
	if( CheckViewSpace(lNeedSize) )
	{
		long lWritePos = *(long*)(pMapView);
		long lReadPos = *(long*)((char*)pMapView + 4);
		if( lWritePos == lReadPos )
			SetEvent(hEvent);
		lWritePos = WriteData(lWritePos, (char*)&lDataSize, sizeof(long));
		for( long i = 0; i < lBlockSize; ++i )
		{
			pDB = DataBlocks[i];
			lWritePos = WriteData(lWritePos, (char*)pDB->Base(), pDB->GetCurSize());	
		}
		*(long*)(pMapView) = lWritePos;
		eRet = SendMsg_Ok;
	}
	else
	{
		eRet = SendMsg_NoSpace;
		static int lNo = 0;
		if (++lNo % ERROR_REPORT_FRQ == 1 && lNo < MAX_ERROR_REPORT)
		{
			//PutDebugString(NET_MODULE, "�ڴ�ӳ���ļ��ռ䲻��, �������: %d. ErrorID: %lu.", lNo, GetLastError());
		}
	}
	ReleaseMutex(hMutex);
	return eRet;
}

// д��Ϣ����
void CMapFile::SendMsg(CBaseMessage *pMsg)
{
	long lQueueSize = m_SendMsgs.GetSize();
	static bool bRewrite = true;
	if (lQueueSize > MAX_BLOCK_MSGS)
	{
		if( bRewrite )
		{
			bRewrite = false;
			static int lNo = 0;
			if (++lNo % ERROR_REPORT_FRQ == 1 && lNo < MAX_ERROR_REPORT)
			{
				//PutDebugString(NET_MODULE, "�ȴ������ڴ���Ϣ��������: %ld, �������: %d. ErrorID: %lu.", lQueueSize, lNo, GetLastError());
			}
		}
		//��������һ��,ʹ�����̼߳���������������Ϣ
		SetEvent(m_hSendEvent);
	}
	else
	{
		bRewrite = true;
		CBaseMessage *pSendMsg = CBaseMessage::AllocMsg();
		pMsg->Clone(pSendMsg);
		// ����ط�֮���Լ�������һ�У���Ҫ��Ϊ��Ӧ���¼ӵ�Validate()��ȫ��飬
		// ���������Ϣ���͵ĵط���server�Ϳͻ��˶�Ҫ�޸ġ�
		pSendMsg->SetTotalSize();
		m_SendMsgs.PushMessage(pSendMsg);
		SetEvent(m_hSendEvent);
	}

	pMsg->UnInit();
}


void CMapFile::SyncSendMsg(CBaseMessage* pMsg)
{
	SendMsgimmediately(pMsg);
	pMsg->UnInit();
}

// ����Ϣ����
CBaseMessage* CMapFile::RecvMsg(void)
{
	HANDLE hMutex = NULL;
	HANDLE hEvent = NULL;
	LPVOID pMapView = NULL;
	if (m_eType & FMT_Client)
	{
		pMapView = m_pszMapViewS;
		hMutex = m_hMutexS;
		hEvent = m_hEventS;
	}
	else if (m_eType & FMT_Server)
	{
		pMapView = m_pszMapViewC;
		hMutex = m_hMutexC;
		hEvent = m_hEventC;
	}
	else
	{
		//PutDebugString(NET_MODULE, "�ڴ�ӳ������������ô���(%d). ErrorID: %lu.", m_eType, GetLastError());
		assert(0);
	}

	WaitForSingleObject(hMutex, INFINITE);

	long lWritePos = *(long*)(pMapView);
	long lReadPos = *(long*)((char*)pMapView + 4);

	while (lWritePos == lReadPos)
	{
		ReleaseMutex(hMutex);
		WaitForSingleObject(hEvent, INFINITE);
		if (m_bExit)
		{
			//PutDebugString(NET_MODULE, "�����߳��˳�. ErrorID: %lu.", GetLastError());
			return NULL;
		}

		WaitForSingleObject(hMutex, INFINITE);
		lWritePos = *(long*)(pMapView);
		lReadPos = *(long*)((char*)pMapView + 4);
	}

	// ����������
	long lDataSize = 0;
	lReadPos = ReadData(lReadPos, (char*)&lDataSize, sizeof(long));
	vector<CDataBlock*> DataBlocks;
	CDataBlock* pDB = NULL;
	long lCurSize = 0;
	if (lDataSize > 10e6)
	{
		//PutDebugString(NET_MODULE, "%s��ȡ�ڴ���Ϣ��С����(%ld), ����.", m_szCurDir, lDataSize);
		ReleaseMutex(hMutex); 
		return NULL;
	}

	while  (lDataSize > 0  && m_eType != FMT_No)
	{
		pDB = m_pDBAllocator->AllocDB(2);
		lCurSize = ( lDataSize > pDB->GetMaxSize() ) ? pDB->GetMaxSize() : lDataSize;
		lDataSize -= lCurSize;
		lReadPos = ReadData(lReadPos, (char *)pDB->Base(), lCurSize);
		pDB->SetCurSize(lCurSize);
		DataBlocks.push_back(pDB);
	}
	*(long*)((char*)pMapView+4) = lReadPos;
	ReleaseMutex(hMutex);

	CBaseMessage* pMsg = CBaseMessage::AllocMsg();
	pMsg->Init(DataBlocks, NULL, FALSE);
	return pMsg;
}

void CMapFile::ClosePassiveFiles(void)
{
	DWORD dwCurTime = timeGetTime();
	// �رճ���û��д���ݵ��ļ�
	ITR_MAP_FILE it = m_Files.begin();
	tagFileInfo0x *pFileInfo = NULL;
	while (it != m_Files.end())
	{
		pFileInfo = it->second;
		if (pFileInfo != NULL)
		{
			if (dwCurTime - pFileInfo->dwLastWriteTime > 8000 &&
				pFileInfo->pFile != NULL)
			{
				//PutDebugString(NET_MODULE, "�ļ�(%s)��ʱ��δ����, �ر��ļ�.", pFileInfo->strFileName.c_str());

				fclose(pFileInfo->pFile);
				pFileInfo->pFile = NULL;
				SAFE_DELETE(pFileInfo);
				it = m_Files.erase(it);
			}
			else
			{
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}

// �����ļ����õ���Ӧ�ļ����
tagFileInfo0x* CMapFile::GetFile(const char* pszFileName)
{
	ITR_MAP_FILE it = m_Files.find(pszFileName);
	if( it != m_Files.end() )
	{
		return it->second;
	}
	return NULL;
}

// ����ļ����
tagFileInfo0x* CMapFile::AddFile(const char* pszFileName)
{
	tagFileInfo0x *pFileInfo = new tagFileInfo0x;
	pFileInfo->dwLastWriteTime = timeGetTime();
	pFileInfo->strFileName = GetCurDir() + string(pszFileName);

	// �򿪸��ļ�
	pFileInfo->pFile = fopen(pFileInfo->strFileName.c_str(), "a+b");
	if (pFileInfo->pFile != NULL)
	{
		m_Files.insert(make_pair(pszFileName, pFileInfo));
	}
	else
	{
		//PutDebugString(NET_MODULE,"Fatal error: AddFileʧ�ܣ�");
		SAFE_DELETE(pFileInfo);
	}

	return pFileInfo;
}

// д�ļ�
void CMapFile::WriteFile(const char* pszFileName, const char* pszContent)
{
	char pszFormatContent[MAX_CONTENT_LEN+256] = "";
	tagFileInfo0x* pFileInfo = GetFile(pszFileName);	
	if (!pFileInfo || !pFileInfo->pFile)
	{
		char pszErrorInfo[1024]="";
		if (pFileInfo != NULL)
		{
			_snprintf(pszErrorInfo,1024,"�ļ�(%s%s)������, ��tagFileInfo0x�ṹ����, ɾ���ṹ.", m_szCurDir, pszFileName);
			m_Files.erase(pszFileName);
			SAFE_DELETE(pFileInfo);
		}
		else
		{
			_snprintf(pszErrorInfo,1024,"�ļ�(%s%s)������, ����ļ�.", m_szCurDir, pszFileName);
		}
		//PutDebugString(NET_MODULE,pszErrorInfo);
		pFileInfo = AddFile(pszFileName);
	}

	if (pFileInfo && pFileInfo->pFile)
	{
		pFileInfo->dwLastWriteTime = timeGetTime();
		if (fseek(pFileInfo->pFile, 0, SEEK_END) != 0)
		{
			//PutDebugString(NET_MODULE, "д�ļ�(%s%s)����. FILE==NULL. ErrorID:%d.", m_szCurDir, pszFileName, errno);
		}
		else
		{
			SYSTEMTIME stTime;
			GetLocalTime(&stTime);
			_snprintf(pszFormatContent,MAX_CONTENT_LEN+256, "\n(%02d-%02d %02d:%02d:%02d):%s",stTime.wMonth, stTime.wDay,
				stTime.wHour, stTime.wMinute, stTime.wSecond,pszContent);
			if( NULL == pFileInfo->pFile ||
				_fileno(pFileInfo->pFile) < 0)
			{
				//PutDebugString(NET_MODULE, "д�ļ�(%s%s)����. FILE==NULL. ErrorID:%d.", m_szCurDir, pszFileName, errno);
			}
			else if( 1 != fwrite(pszFormatContent,strnlen(pszFormatContent,MAX_CONTENT_LEN+256),1,pFileInfo->pFile) )
			{
				//PutDebugString(NET_MODULE, "д�ļ�(%s%s)����. failed writen. ErrorID:%d.", m_szCurDir, pszFileName, errno);
			}
		}
	}
	return;
}

// ����ļ�
void CMapFile::ClearFile(const char* pszFileName)
{
	tagFileInfo0x* pFileInfo = GetFile(pszFileName);

	// ����ļ����ڣ���رո��ļ�.
	if(pFileInfo && pFileInfo->pFile)
	{
		pFileInfo->pFile = freopen(pFileInfo->strFileName.c_str(),
			"w+b",
			pFileInfo->pFile);
	}
	return;
}

// �����ļ����õ���Ӧ�ļ����
void CMapFile::CloseFile(const char* pszFileName)
{
	ITR_MAP_FILE it = m_Files.find(pszFileName);
	if (it != m_Files.end())
	{
		// ����ļ����ڣ���رո��ļ�.
		tagFileInfo0x *pFileInfo = it->second;
		if (pFileInfo != NULL)
		{
			if (pFileInfo->pFile != NULL)
			{
				fclose(pFileInfo->pFile);
				pFileInfo->pFile = NULL;
			}
			SAFE_DELETE(pFileInfo);
		}

		m_Files.erase(it);
	}
}

// ���õ�ǰ�̶߳�д·��
void CMapFile::SetCurDir(const char* szDir)
{
	if (strcpy_s(m_szCurDir, szDir) != 0)
	{
		//PutDebugString(NET_MODULE, "���õ�ǰ����Ŀ¼(%s)����. ErrorID: %lu.", szDir, errno);
	}

	CreateDirectory(m_szCurDir, NULL);
}

// ��ȡ��ǰ�̶߳�д·��
const char* CMapFile::GetCurDir(void)
{
	return m_szCurDir;
}