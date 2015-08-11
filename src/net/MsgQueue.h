#pragma once


#include <deque>
#include <mutex>

class CBaseMessage;
typedef std::deque<CBaseMessage*> MsgQueue;

/* -------------------------------------------
  [CMsgQueue] ��Ϣ����

  ����:
	�洢���յ�����Ϣ���ȴ�����

---------------------------------------------*/
class CMsgQueue  
{
public:
	CMsgQueue();
	~CMsgQueue();

	bool            PushMessage(CBaseMessage* pMsg);	// ѹ����Ϣ
	CBaseMessage*   PopMessage();				        // ������Ϣ
    bool            Splice(MsgQueue& other);
	void	        GetAllMessage(MsgQueue& tmp);

	size_t          GetSize();		// �õ���Ϣ���г���
	void            Clear();		// �����Ϣ

private:
    MsgQueue    queue_;	// ����
    std::mutex  mutex_;
};

