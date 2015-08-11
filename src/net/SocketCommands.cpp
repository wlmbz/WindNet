//////////////////////////////////////////////////////////////////////////
//SocketCommands.cpp
//Fun:对Socket的操作命令列表
//Create Time:2004.12.17
//Author:wangqiao
//////////////////////////////////////////////////////////////////////////


#include "SocketCommands.h"



CSocketCommands::CSocketCommands(void)
{
}

CSocketCommands::~CSocketCommands(void)
{
	Clear();
}


// 压入命令到队列后段
bool CSocketCommands::PushBack(tagSocketOper* cmd)
{
    if (cmd)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        commands_.push_back(cmd);
        return true;
    }
	return false;
}

// 得到命令队列长度
size_t CSocketCommands::GetSize()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return commands_.size();
}

// 清空命令队列
void CSocketCommands::Clear()
{
    std::lock_guard<std::mutex> guard(mutex_);
    commands_.clear();
}

void CSocketCommands::CopyAllCommand(CommandsQueue& tmp)
{
    std::lock_guard<std::mutex> guard(mutex_);
    tmp.swap(commands_);
}