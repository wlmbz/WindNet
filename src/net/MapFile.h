//=============================================================================
/**
*  file: MapFile.h
*
*  Brief:ͨ��Windows���ļ�Ӱ����ƣ���װ�˼򵥵�Ӱ���ļ���������
*			�����ļ���־��¼
*
*  Authtor:wangqiao
*	
*	Datae:2008-10-29
*/
//=============================================================================

#pragma once

#include <stdio.h>
#include <map>
#include <string>
#include "DefType.h"
#include "MsgQueue.h"

class CDataBlock;
class CBaseMessage;
class CDataBlockAllocator;

const long MAX_FILENAME_LEN = 256;
const long MAX_CONTENT_LEN = 1024*10;
const long MAX_MAP_VIEW_SIZE = 1048576*10;

//Ӱ���ļ��Ĵ򿪷�ʽ
enum eFileMapServerType
{
	FMT_No = 0,
	FMT_Client=0x00000001,//�ͻ���ʽ
	FMT_Server=0x00000002,//����ʽ
};

enum MSG_DEFAULT_TYPE
{
	MSG_MEM_OPEN_FILE		= 0x0001,		// ���ļ�
	MSG_MEM_WRITE_FILE		= 0x0002,		// д���ļ�
	MSG_MEM_CLEAR_FILE		= 0x0003,		// ����ļ�
	MSG_MEM_CLOSE_FILE		= 0x0004,		// �ر��ļ�
};

//��Ϣ���ļ��Ĳ�����ʽ
enum eFileOpType
{
	File_No		= 0,
	File_Open	= 1,	// ���ļ�
	File_Write	= 2,	// д�ļ�
	File_Clear	= 3,	// ����ļ�
	File_Close	= 4,	// �ر��ļ�
	End=0xFF,//��������,�˳��������߳�
};

struct tagFileInfo0x
{	
	tagFileInfo0x(void)
		: dwLastWriteTime(0)
		, pFile(NULL)
	{
	}

    std::string strFileName;		// �ļ�·��
	DWORD dwLastWriteTime;	// ���һ�β���ʱ��
	FILE* pFile;			// �ļ����
};

typedef std::map<std::string, tagFileInfo0x*> MAP_FILES;
typedef MAP_FILES::iterator ITR_MAP_FILE;

class CMapFile
{
public:
	CMapFile(void);
	~CMapFile(void);

private:
	eFileMapServerType m_eType;			// Ӱ���������
	HANDLE	m_hFile;					// Ӱ�䵽��ַ�ռ���ļ����
	HANDLE	m_hMapObject;				// Ӱ���ļ����
	LPVOID m_pszMapViewS;				// �ڴ�Ӱ���ļ��ĵ�ַ�ռ�(for Server)
	LPVOID m_pszMapViewC;				// �ڴ�Ӱ���ļ��ĵ�ַ�ռ�(for Client)
	HANDLE m_hMutexS;					// ���ڶ�д�Ļ�����(for Server)
	HANDLE m_hEventS;					// ������¼����(for Server)
	HANDLE m_hMutexC;					// ���ڶ�д�Ļ�����(for Client)
	HANDLE m_hEventC;					// ������¼����(for Client)
	CDataBlockAllocator* m_pDBAllocator;
	MAP_FILES m_Files;					// д���ݵ������ļ����
	char m_szCurDir[256];				// ��ǰ����Ŀ¼
	bool volatile m_bExit;				// �߳��˳���־
	HANDLE m_hSendEvent;				// ������Ϣ�¼�֪ͨ
	CMsgQueue m_SendMsgs;				// ���յ�����Ϣ����

	//��װ�˶�д���ݵĹ���
private:
	// �ͷ������ļ�
	void ReleaseAllFiles(void);
	//���ռ��С�Ƿ�����
	bool CheckViewSpace(long lNeedSize);
	//�����´�д���ݵ�λ��
	int WriteData(int nPos,const char *pData,int nLen);
	//��Χ�´ζ����ݵ�λ��
	int ReadData(int nPos,char *pData,int nLen);
	void SendExitMsg(void);						// �����߳��˳�

	enum eRetSendMsg
	{
		//���ͳɹ�
		SendMsg_Ok = 1,
		//����ʧ��,Ӱ���ļ��ռ䲻��
		SendMsg_NoSpace,
		//��������
		SendMsg_Error,
	};
	//��ⷢ��һ����Ϣ
	eRetSendMsg SendMsgimmediately(CBaseMessage* pMsg);

public:
	void SetDBAllocator(CDataBlockAllocator* pDBA);
	//��ʼ��
	bool Initial(eFileMapServerType eFMType,const char* pszMapObjName,const char* pFileName=NULL);
	//��������
	bool Reset(eFileMapServerType eFMType,const char* pszMapObjName,const char* pFileName=NULL);
	void Reset(void);

	//�ͷ���Դ
	void Release();
	void Exit(void);
	long SendMsg(void);							// д��Ϣ(�߳�ͳһ����), ���ط�����Ϣ����
	void SendMsg(CBaseMessage* pMsg);			// д��Ϣ����
	// ͬ��д��Ϣ����,����Ϣ������ʱ,����Ϣ�п��ܶ���
	void SyncSendMsg(CBaseMessage* pMsg);		
	CBaseMessage* RecvMsg(void);				// ����Ϣ����

	// �رճ�ʱ��(8s)û��I/O�������ļ�
	void ClosePassiveFiles(void);

	// �����ļ����õ���Ӧ�ļ����
	tagFileInfo0x* GetFile(const char* pszFileName);

	// ����ļ����: pszFileName����ΪNULL
	tagFileInfo0x* AddFile(const char* pszFileName);

	// д�ļ�
	void WriteFile(const char* pszFileName, const char* pszContent);

	// ����ļ�
	void ClearFile(const char* pszFileName);

	// �����ļ����ر���Ӧ�ļ����
	void CloseFile(const char* pszFileName);

	// ���õ�ǰ�̶߳�д·��
	void SetCurDir(const char* szDir);

	// ��ȡ��ǰ�̶߳�д·��
	const char* GetCurDir(void);
};

