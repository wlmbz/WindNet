//////////////////////////////////////////////////////////////////////////
//SocketCommands.cpp
//Fun:��Socket�Ĳ��������б�
//Create Time:2004.12.17
//Author:wangqiao
//////////////////////////////////////////////////////////////////////////


#include "socketcommands.h"



CSocketCommands::CSocketCommands(void)
{
	InitializeCriticalSection(&m_CriticalSectionCommans);
	m_hWait = CreateEvent(NULL,FALSE,FALSE,0);
	m_bWait = false;
}

CSocketCommands::~CSocketCommands(void)
{
	Clear();
	DeleteCriticalSection(&m_CriticalSectionCommans);
	CloseHandle(m_hWait);
}


// ѹ��������к��
bool CSocketCommands::Push_Back(tagSocketOper* pCommand)
{
	if(NULL == pCommand)	return false;

	EnterCriticalSection(&m_CriticalSectionCommans);
	if(m_Commands.size() == 0)
		SetEvent(m_hWait);
	m_Commands.push_back(pCommand);
	LeaveCriticalSection(&m_CriticalSectionCommans);

	return false;
}

// �õ�������г���
long CSocketCommands::GetSize()
{
	EnterCriticalSection(&m_CriticalSectionCommans);
	long lSize = (long)m_Commands.size();
	LeaveCriticalSection(&m_CriticalSectionCommans);
	return lSize;
}

// ����������
void CSocketCommands::Clear()
{
	EnterCriticalSection(&m_CriticalSectionCommans);
	m_Commands.clear();
	LeaveCriticalSection(&m_CriticalSectionCommans);
}

void CSocketCommands::CopyAllCommand(CommandsQueue& TemptCommandsQueue)
{
	EnterCriticalSection(&m_CriticalSectionCommans);
	while(m_Commands.size() == 0)
	{
		LeaveCriticalSection(&m_CriticalSectionCommans);
		//�ȴ�֪ͨ�¼�
		WaitForSingleObject(m_hWait,INFINITE);
		EnterCriticalSection(&m_CriticalSectionCommans);
	}
	TemptCommandsQueue = m_Commands;
	m_Commands.clear();
	LeaveCriticalSection(&m_CriticalSectionCommans);
}