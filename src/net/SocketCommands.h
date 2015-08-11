//////////////////////////////////////////////////////////////////////////
//SocketCommands.h
//Fun:对Socket的操作命令列表
//Create Time:2004.12.17
//Author:wangqiao
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>
#include <mutex>
#include "common/DefType.h"



//对网络操作的类型
enum eSocketOperaType
{
	//完成端口用到的
	SCOT_Init,						//初始化
	SCOT_DisConn,					//断开某个连接
	SCOT_DisConnAll,				//断开所有连接
	SCOT_CloseAll,					//关闭所有连接
	SCOT_Send,						//发送消息

	SCOT_OnSend,					//完成端口上发送结束消息
	SCOT_OnRecieve,					//接受消息完成
	SCOT_OnSendZeroByte,			//发送0字节大小的操作
	SCOT_OnClose,					//调节子被关系
	SCOT_OnError,					//发生错误

	SCOT_ExitThread,				//退出运行线程
	
	//事件客户端用到的
	SCOT_Client_Send,
	SCOT_Client_OnSend,				//当客户端可以发送数据时激活
	SCOT_Client_OnRecv,				//当客户端有数据来的时候
	SCOT_Client_ExitThread,			//退出运行的主线程
};

//对socket的操作结构，大小:24Byte
struct tagSocketOper
{
	eSocketOperaType    OperaType;
	uint32_t	        lSocketID = 0;
	char*		        pStrID = nullptr;		//字符串ID
	void*			    pBuf = nullptr;			//操作的内容
	int32_t			    lNum1 = 0;				//备用值1
	int32_t			    lNum2 = 0;				//备用值1


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

//此类的消费线程只能有一个
class CSocketCommands
{
public:
	CSocketCommands();
	~CSocketCommands();

	bool    PushBack(tagSocketOper* cmd);	// 压入命令到队列后段
	size_t  GetSize();						// 得到命令队列长度
	void    Clear();						// 清空命令队列]
	void    CopyAllCommand(CommandsQueue& tmp);

private:
    CommandsQueue	commands_;				//命令操作队列
    std::mutex      mutex_;
};
