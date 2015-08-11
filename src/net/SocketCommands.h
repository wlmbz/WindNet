//////////////////////////////////////////////////////////////////////////
//SocketCommands.h
//Fun:��Socket�Ĳ��������б�
//Create Time:2004.12.17
//Author:wangqiao
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>
#include <mutex>
#include "common/DefType.h"



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
	eSocketOperaType    OperaType;
	uint32_t	        lSocketID = 0;
	char*		        pStrID = nullptr;		//�ַ���ID
	void*			    pBuf = nullptr;			//����������
	int32_t			    lNum1 = 0;				//����ֵ1
	int32_t			    lNum2 = 0;				//����ֵ1


    void Init(eSocketOperaType type, uint32_t lID, void* buf, 
              uint32_t num1 = 0, uint32_t num2 = 0)
	{
		OperaType = type;
        lSocketID = lID;
        pBuf = buf;
        lNum1 = num1; 
        lNum2 = num2;
	}

};

typedef std::deque<tagSocketOper*> CommandsQueue;

//����������߳�ֻ����һ��
class CSocketCommands
{
public:
	CSocketCommands();
	~CSocketCommands();

	bool    PushBack(tagSocketOper* cmd);	// ѹ��������к��
	size_t  GetSize();						// �õ�������г���
	void    Clear();						// ����������]
	void    CopyAllCommand(CommandsQueue& tmp);

private:
    CommandsQueue	commands_;				//�����������
    std::mutex      mutex_;
};
