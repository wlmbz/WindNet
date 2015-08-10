#pragma once

#include <map>
#include "BaseMessage.h"
#include "common/DefType.h"

class CMsgDescribe
{
private:
    std::map<ulong, std::string>								m_mapMsg;

	CMsgDescribe									();
	~CMsgDescribe									();

public:
	//##设置全局指针
	static CMsgDescribe*	g_pMsgDesc;

	static CMsgDescribe*	GetInstance				();
	static void				FinalRelease			();

	const CHAR*				GetDesc					( ulong dwID );
};
