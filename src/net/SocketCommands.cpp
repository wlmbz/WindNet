//////////////////////////////////////////////////////////////////////////
//SocketCommands.cpp
//Fun:��Socket�Ĳ��������б�
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


// ѹ��������к��
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

// �õ�������г���
size_t CSocketCommands::GetSize()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return commands_.size();
}

// ����������
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