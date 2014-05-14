
#include "MsgQueue.h"
#include "basemessage.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsgQueue::CMsgQueue()
{
	InitializeCriticalSection(&m_CriticalSectionMsgQueue);
}

CMsgQueue::~CMsgQueue()
{
	Clear();
	DeleteCriticalSection(&m_CriticalSectionMsgQueue);
}

//ѹ��һ����Ϣ
bool CMsgQueue::PushMessage(CBaseMessage* pMsg)
{
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	if( pMsg )
	{
		m_msgQueue.push_back(pMsg);
		LeaveCriticalSection(&m_CriticalSectionMsgQueue);
		return true;
	}

	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
	return false;
}

bool CMsgQueue::PushMsgsoFront(msgQueue& queMsgs)
{
	if( queMsgs.size() == 0)	return false;
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	m_msgQueue.insert(m_msgQueue.begin(),queMsgs.begin(),queMsgs.end());
	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
	return true;
}

//����һ����Ϣ
CBaseMessage* CMsgQueue::PopMessage()
{
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	if (m_msgQueue.empty())
	{
		LeaveCriticalSection(&m_CriticalSectionMsgQueue);
		return NULL;
	}

	CBaseMessage* pMsg = *m_msgQueue.begin();
	m_msgQueue.pop_front();
	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
	return pMsg;
}

void CMsgQueue::GetAllMessage(msgQueue& pTemptMsgQueue)
{
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	pTemptMsgQueue = m_msgQueue;
	m_msgQueue.clear();
	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
}

// �õ���Ϣ���г���
long CMsgQueue::GetSize()
{
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	long lSize = (long)m_msgQueue.size();
	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
	return lSize;
}


//�����Ϣ
void CMsgQueue::Clear()
{
	EnterCriticalSection(&m_CriticalSectionMsgQueue);
	for (msgQueue::iterator it = m_msgQueue.begin(); it!=m_msgQueue.end(); it++)
	{
		delete (*it);
	}
	m_msgQueue.clear();
	LeaveCriticalSection(&m_CriticalSectionMsgQueue);
}