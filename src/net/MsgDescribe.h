#ifndef _MSG_DESCRIBUTE
#include <map>
#include "BaseMessage.h"

class CMsgDescribe
{
private:
	map<ulong,string>								m_mapMsg;

	CMsgDescribe									();
	~CMsgDescribe									();

public:
	//##����ȫ��ָ��
	static CMsgDescribe*	g_pMsgDesc;

	static CMsgDescribe*	GetInstance				();
	static void				FinalRelease			();

	const CHAR*				GetDesc					( ulong dwID );
};

#endif