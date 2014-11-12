// Fox@20090119----------------------------------------------
// File:    MemClient.h
// Brief:   CMemClient��Ϊclient���ڴ�ӳ���ļ����в���.
//
// Author:  Fox (yulefox@gmail.com)
// Date:    Jan.19, 2009
// Fox@20090119----------------------------------------------

#pragma once

#include "MsgQueue.h"
#include "MapFile.h"

class CBaseMessage;
class CDataBlockAllocator;

class CMemClient
{
public:
	CMemClient(void);
	~CMemClient(void);

	bool Initial(const char* szMapObjName,
		CDataBlockAllocator* pDBAllocator);
	bool Release(void);
	CMsgQueue* GetRecvMsgs(void) { return &m_RecvMsgs; }
	bool Send(CBaseMessage* pMsg,
		eFileMapServerType eType = FMT_Server);
	//ͬ��������Ϣ��(����Ϣ������ʱ,�п��ܶ�������Ϣ)
	bool SyncSend(CBaseMessage* pMsg,
		eFileMapServerType eType = FMT_Server);
	void AsyOpenFile(const char* pszFileName);		// ���ļ�
	void AsyCloseFile(const char* pszFileName);		// �ر��ļ�
	void AsyClearFile(const char* pszFileName);		// ����ļ�����
	void AsyWriteFile(const char* pszFileName,
		const char* pszContent);					// �첽д�ļ��Ľӿ�
	friend unsigned __stdcall ProcFunc(void* pArguments);
	friend unsigned __stdcall SendMsgFunc(void* pArguments);

private:
	bool Recv(void);
	void Send(void);

	HANDLE			m_hProcThread;
	HANDLE			m_hSendMsgThread;
	CMapFile*		m_pMapFile;
	CMsgQueue		m_RecvMsgs;
};