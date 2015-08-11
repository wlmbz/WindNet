
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

//压入一个消息
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

//弹出一个消息
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

// 得到消息队列长度
size_t CMsgQueue::GetSize()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return queue_.size();
}


//清空消息
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