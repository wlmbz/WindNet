// Fox@20090119----------------------------------------------
// File:    MemServer.h
// Brief:   CMemServer��Ϊserver����client(s)�ڴ�ӳ���ļ�.
//
// Author:  Fox (yulefox@gmail.com)
// Date:    Jan.19, 2009
// Fox@20090119----------------------------------------------

#pragma once

#include "MsgQueue.h"
#include "MapFile.h"

class CBaseMessage;
class CDataBlockAllocator;

// ӳ���ļ�����
struct tagConf
{
	tagConf(void)
		: dwID(0)
		, hProcThread(NULL)
		, pMapFile(NULL)
	{
		memset(szMapObj, 0, 64);
		memset(szMapFile, 0, 64);
		memset(szOutputDir, 0, 64);
		lTestID = -1;
	}

	const tagConf& operator=(const tagConf& rConf)
	{
		memcpy(this, &rConf, sizeof(rConf));
		return *this;
	}

	DWORD		dwID;
	HANDLE		hProcThread;
	HANDLE		hSendMsgThread;
	CMapFile*	pMapFile;
	char		szMapObj[64];
	char		szMapFile[64];
	char		szOutputDir[64];

	long		lTestID;
};

typedef std::map<DWORD, tagConf> MF_CONFS;
typedef MF_CONFS::iterator ITR_MF_CONF;

class CMemServer
{
public:
	CMemServer(void);
	~CMemServer(void);

	unsigned AddServer(DWORD dwID,
		const char* szMapObj,
		const char* szMapFile,
		const char* szOutputDir);					// ���SERVER, �����߳�ID.
	bool Initial(CDataBlockAllocator* pDBAllocator);
	bool Release(void);
	CMsgQueue* GetRecvMsgs(void) { return &m_RecvMsgs; }
	bool Send(DWORD dwID,
		CBaseMessage* pMsg,
		eFileMapServerType eType = FMT_Client);
	friend unsigned __stdcall ProcFuncS(void* pArguments);
	friend unsigned __stdcall SendMsgFuncS(void* pArguments);

	void Reset(DWORD dwID);
	void ResetAll(void);

private:
	bool Run(DWORD dwID);
	void Send(DWORD dwID);									// �����߳�ID������Ϣ
	// �ļ���������
	tagFileInfo0x* GetFile(DWORD dwID,
		const char* pszFileName);							// �����ļ����õ���Ӧ�ļ����
	tagFileInfo0x* AddFile(DWORD dwID,
		const char* pszFileName);							// ���һ���ļ�
	void OpenFile(DWORD dwID, const char* pszFileName);		// ���ļ�
	void CloseFile(DWORD dwID, const char* pszFileName);	// �ر��ļ�
	void ClearFile(DWORD dwID, const char* pszFileName);	// ����ļ�
	void WriteFile(DWORD dwID, const char* pszFileName,
		const char* pszContent);							// д�ļ�
	CMapFile* GetMapFile(DWORD dwID);						// �����߳�ID��ȡMapFile����

private:
	MF_CONFS		m_MapFiles;
	CMsgQueue		m_RecvMsgs;
	CDataBlockAllocator*	m_pDBAllocator;
};
