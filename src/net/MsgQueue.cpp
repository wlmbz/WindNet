
#include "MsgQueue.h"
#include "BaseMessage.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsgQueue::CMsgQueue()
{
}

CMsgQueue::~CMsgQueue()
{
	Clear();
}

//ѹ��һ����Ϣ
bool CMsgQueue::PushMessage(CBaseMessage* pMsg)
{
	if( pMsg )
	{
        std::lock_guard<std::mutex> guard(mutex_);
        queue_.push_back(pMsg);
		return true;
	}
	return false;
}

bool CMsgQueue::Splice(MsgQueue& other)
{
    if (other.empty())
    {
        std::lock_guard<std::mutex> guard(mutex_);
        queue_.insert(queue_.begin(), other.begin(), other.end());
        return true;
    }
    return false;
}

//����һ����Ϣ
CBaseMessage* CMsgQueue::PopMessage()
{
    std::lock_guard<std::mutex> guard(mutex_);
    CBaseMessage* pMsg = NULL;
    if (!queue_.empty())
    {
        pMsg = queue_.front();
        queue_.pop_front();
    }
    return pMsg;
}

void CMsgQueue::GetAllMessage(MsgQueue& tmp)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.swap(tmp);
}

// �õ���Ϣ���г���
size_t CMsgQueue::GetSize()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return queue_.size();
}


//�����Ϣ
void CMsgQueue::Clear()
{
    MsgQueue tmp;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        tmp.swap(queue_);
    }
    for (auto pMsg : tmp)
    {
        delete pMsg;
    }
}