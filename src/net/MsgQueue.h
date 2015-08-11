#pragma once


#include <deque>
#include <mutex>

class CBaseMessage;
typedef std::deque<CBaseMessage*> MsgQueue;

/* -------------------------------------------
  [CMsgQueue] 消息队列

  功能:
	存储接收到的消息，等待处理。

---------------------------------------------*/
class CMsgQueue  
{
public:
	CMsgQueue();
	~CMsgQueue();

	bool            PushMessage(CBaseMessage* pMsg);	// 压入消息
	CBaseMessage*   PopMessage();				        // 弹出消息
    bool            Splice(MsgQueue& other);
	void	        GetAllMessage(MsgQueue& tmp);

	size_t          GetSize();		// 得到消息队列长度
	void            Clear();		// 清空消息

private:
    MsgQueue    queue_;	// 队列
    std::mutex  mutex_;
};

