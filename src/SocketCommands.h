//////////////////////////////////////////////////////////////////////////
//SocketCommands.h
//Fun:��Socket�Ĳ��������б�
//Create Time:2004.12.17
//Author:wangqiao
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <deque>
#include "base/DefType.h"

class CServerClient;

//���������������
enum eSocketOperaType
{
	//��ɶ˿��õ���
	SCOT_Init,						//��ʼ��
	SCOT_DisConn,					//�Ͽ�ĳ������
	SCOT_DisConnAll,				//�Ͽ���������
	SCOT_CloseAll,					//�ر���������
	SCOT_Send,						//������Ϣ

	SCOT_OnSend,					//��ɶ˿��Ϸ��ͽ�����Ϣ
	SCOT_OnRecieve,					//������Ϣ���
	SCOT_OnSendZeroByte,			//����0�ֽڴ�С�Ĳ���
	SCOT_OnClose,					//�����ӱ���ϵ
	SCOT_OnError,					//��������

	SCOT_ExitThread,				//�˳������߳�
	
	//�¼��ͻ����õ���
	SCOT_Client_Send,
	SCOT_Client_OnSend,				//���ͻ��˿��Է�������ʱ����
	SCOT_Client_OnRecv,				//���ͻ�������������ʱ��
	SCOT_Client_ExitThread,			//�˳����е����߳�
};

//��socket�Ĳ����ṹ����С:24Byte
struct tagSocketOper
{
	eSocketOperaType OperaType;
	ulong	 lSocketID;
	char*			 pStrID;					//�ַ���ID

	void*			 pBuf;						//����������
	long			 lNum1;						//����ֵ1
	long			 lNum2;						//����ֵ1
	//CServerClient*	 pServerClient;			//�ͻ����׽���
	
	tagSocketOper(){}

	tagSocketOper(eSocketOperaType eOperType,ulong lID,void* pTemptBuf,
		long lNumber1,long lNumber2=0)
		:OperaType(eOperType)
		,lSocketID(lID)
		,pStrID(NULL)
		,pBuf(pTemptBuf)
		,lNum1(lNumber1)
		,lNum2(lNumber2)
	{
	}


	tagSocketOper(eSocketOperaType eOperType,ulong lID,char* pBufID,void* pTemptBuf,
		long lNumber1,long lNumber2=0)
		:OperaType(eOperType)
		,lSocketID(lID)
		,pStrID(pBufID)
		,pBuf(pTemptBuf)
		,lNum1(lNumber1)
		,lNum2(lNumber2)
	{
	}

	void Init(eSocketOperaType eOperType,ulong lID,void* pTemptBuf,
		long lNumber1 = 0,long lNumber2=0)
	{
		OperaType = eOperType;lSocketID = lID;pBuf=pTemptBuf;
		lNum1 = lNumber1;lNum2 = lNumber2;
	}
};

typedef std::deque<tagSocketOper*> CommandsQueue;

//����������߳�ֻ����һ��

class CSocketCommands
{
private:
	CommandsQueue	 m_Commands;				//�����������
	CRITICAL_SECTION m_CriticalSectionCommans;
	//�Ƿ����̵߳ȴ�
	bool	m_bWait;
	//������Ϊ�յ�ʱ��,��Ӧ�̵߳ĵȴ��¼�ͬ־
	HANDLE	m_hWait;
public:
	CSocketCommands(void);
	~CSocketCommands(void);

	bool Push_Back(tagSocketOper* pCommand);	// ѹ��������к��

	long GetSize();								// �õ�������г���
	void Clear();								// ����������]

	void CopyAllCommand(CommandsQueue& TemptCommandsQueue);
};
