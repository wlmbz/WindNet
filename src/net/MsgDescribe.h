#pragma once

#include <map>
#include "BaseMessage.h"
#include "common/DefType.h"

class CMsgDescribe
{
private:
    std::map<uint32_t, std::string>								m_mapMsg;

	CMsgDescribe									();
	~CMsgDescribe									();

public:
	//##����ȫ��ָ��
	static CMsgDescribe*	g_pMsgDesc;

	static CMsgDescribe*	GetInstance				();
	static void				FinalRelease			();

	const CHAR*				GetDesc					( uint32_t dwID );
};
