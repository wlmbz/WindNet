#pragma once

#include <Windows.h>
#include <deque>

class CBaseMessage;
typedef std::deque<CBaseMessage*> msgQueue;

/* -------------------------------------------
  [CMsgQueue] ��Ϣ����

  ����:
	�洢���յ�����Ϣ���ȴ�����

---------------------------------------------*/
class CMsgQueue  
{
private:
	msgQueue m_msgQueue;	// ����
	CRITICAL_SECTION m_CriticalSectionMsgQueue;

public:
	CMsgQueue();
	virtual ~CMsgQueue();

	bool PushMessage(CBaseMessage* pMsg);	// ѹ����Ϣ
	//ѹ������Ϣ������ǰ
	bool PushMsgsoFront(msgQueue& queMsgs);
	CBaseMessage* PopMessage();				// ������Ϣ
	void	GetAllMessage(msgQueue& pTemptMsgQueue);

	long GetSize();							// �õ���Ϣ���г���
	void Clear();							// �����Ϣ
};

