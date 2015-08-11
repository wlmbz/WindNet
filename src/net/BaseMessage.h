/**
 * @file Public/Nets/BaseMessage.h
 *
 * @brief 游戏的消息都从本类派生包含每个消息的基本内容（大小、类型和时间）
 * 提供基本的组装消息、发送消息功能
 *
 * @authtor wangqiao
 * @date 2005-4-15
 * Modify 2007-4-13,整理了代码和结构，优化了效率
 */

#pragma once

#include <set>
#include <map>
#include <list>
#include <vector>
#include "MsgQueue.h"
#include "DataBlock.h"
#include "DataBlockAllocator.h"
#include "DataBlockSetWriteRead.h"

class CMySocket;
class CMessage;
class CGUID;
struct tagDataBlockWriteSet;
struct tagDataBlockReadSet;

//定义消息的最大优先级
const int MAX_MSG_PRIO = 100;
const int HEAD_SIZE = 4;

enum eBaseMsgType
{
	BASEMSG_Socket_Accept = 1,
	BASEMSG_Socket_Close,
	BASEMSG_Transfer_Congestion,//传输拥塞
	BASEMSG_Transfer_Smoothly,//传输畅通
};

class CBaseMessage
{
public:

	CBaseMessage();
	virtual ~CBaseMessage();

	struct stMsg
	{
		uint32_t lSize;		// 消息大小
		uint32_t eType;		// 消息类型
		uint32_t lVerifyCode;	// 校验码
	};

	////////////////////////////////////////////////////////////////////////
	//	基本数据
	////////////////////////////////////////////////////////////////////////
private:

	//消息的数据部分
	typedef std::vector<CDataBlock*>::iterator itMsgData;
    std::vector<CDataBlock*> m_MsgData;
	
	//总数据大小
	
	tagDBOpParam m_ReadParam;
	////当前读数据块编号
	//long	m_nCurRdNum;
	////当前读数据块的大小
	//long	m_nCurRdDBSize;
	////当前读数据的位置
	//long	m_nCurRdPos;
	////当前读数据块的指针
	//byte	*m_pRdDBPtr;

	tagDBOpParam m_WriteParam;
	////当前写数据块编号
	//long	m_nCurWrNum;
	////当前写数据块的大小
	//long	m_nCurWrDBSize;
	////当前写数据的位置
	//long	m_nCurWrPos;
	////当前写数据块的指针
	//byte	*m_pWrDBPtr;

	long	m_lNetFlag;
	//该消息的优先级别
	long	m_lPriorityLvl;
	//开始发送该消息的事件
	uint32_t	m_dwStartSendTime;
	//消息的引用计数
	long m_lRefCount;

protected:
	byte* Base()
	{
		if(m_MsgData.size() > 0)
			return m_MsgData[0]->Base();
		return NULL;
	}

	byte* GetMsgBuf()
	{
		return m_MsgData[0]->Base()+HEAD_SIZE;
	}
	
	void SetSize(uint32_t l)	{((stMsg*)GetMsgBuf())->lSize = l;}

	void Add(byte*, uint32_t size);
	void* Get(byte*, uint32_t size);
public:
	void Init(uint32_t type);
    void Init(std::vector<CDataBlock*>& MsgData, const byte kn[16][6], bool bDecrypt);
	void UnInit();
	//验证消息内容是否合法
	bool Validate();

	void Clone(CBaseMessage *pMsg);

	void AddWrDataBlock();
	void AddRdDataBlock();

	void SetType(uint32_t t)
	{
        if (m_MsgData.empty() ||
            m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)) )
			return;	
		((stMsg*)GetMsgBuf())->eType = t;
	}

	//void SetTime(uint32_t l)
	//{
	//	if(m_MsgData.size()==0 || m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
	//		return;	
	//	((stMsg*)GetMsgBuf())->lTime = l;
	//}
	void SetCode(uint32_t l)
	{
		if (m_MsgData.empty() ||
            m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
            return;	
		((stMsg*)GetMsgBuf())->lVerifyCode = l;
	}
	uint32_t GetSize()
	{
		if(m_MsgData.size()==0 || m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
			return 0;	
		return ((stMsg*)GetMsgBuf())->lSize;
	}
	uint32_t GetType()
	{
		if(m_MsgData.size()==0 || m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
			return 0;	
		return ((stMsg*)GetMsgBuf())->eType;
	}
	/*uint32_t GetMsgSendTime()
      {
      if(m_MsgData.size()==0 || m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
      return 0;	
      return ((stMsg*)GetMsgBuf())->lTime;
      }*/
	uint32_t GetCode()
	{
		if(m_MsgData.size()==0 || m_MsgData[0]->GetMaxSize() < (HEAD_SIZE+sizeof(stMsg)))
			return 0;		
		return ((stMsg*)GetMsgBuf())->lVerifyCode;
	}

	//得到总大小(包括实际消息大小和消息前的附加头)
	uint32_t GetTotalSize(void);/*
                               {
                               return m_lMsgTotalSize ? m_lMsgTotalSize : (m_lMsgTotalSize = GetSize() + HEAD_SIZE);
                               }*/
	void SetTotalSize() { *((long*)Base()) = GetSize()+HEAD_SIZE; }

    std::vector<CDataBlock*>& GetMsgData() { return m_MsgData; }
    void SetMsgData(std::vector<CDataBlock*>& MsgData)	{ UnInit(); m_MsgData = MsgData; }
	void ClearMsgData()		{m_MsgData.clear();}

	void SetRefCount(long lNum)	{ m_lRefCount = lNum; }
	void AddRefCount(long lNum)	{ m_lRefCount += lNum; }
	long RemoveRefCount()		{ return --m_lRefCount; }
	void Encrypt(const byte kn[16][6]);	/// 对消息进行加密
	void Decrypt(const byte kn[16][6]);	/// 对消息进行解密

	////////////////////////////////////////////////////////////////////////
	//	添加和获取数据
	////////////////////////////////////////////////////////////////////////
public:
	void Add(char);
	void Add(byte);
	void Add(short);
	void Add(uint16_t);
	void Add(long);
	void Add(LONG64);
	void Add(uint32_t);
	void Add(float);
	void Add(const char*);
	void Add(const CGUID&	guid);
	void AddEx(void*, long size);//增加安全性，和GetEx配合使用
	bool GetDBWriteSet(tagDataBlockWriteSet& DBWriteSet);

	virtual void Reset(void);           //清空消息内容

	
	char GetChar();
	byte GetByte();
	short GetShort();
	uint16_t GetWord();
	long GetLong();
	LONG64 GetLONG64();
	uint32_t GetDWord();
	float GetFloat();
	bool  GetGUID(CGUID& guid);
	void* GetEx(void*, long size);
	char* GetStr( char* pStr,long lMaxLen );
	bool GetDBReadSet(tagDataBlockReadSet& DBReadSet);
	
protected:
	bool m_bEncrypted;						/// 消息无需加密或已加密
	uint32_t m_lMsgTotalSize;			/// 防止加密破坏
    typedef std::list<CBaseMessage*>	listBaseMsgs;
    typedef std::list<CBaseMessage*>::iterator itBaseMsg;
	static uint32_t m_nMaxFreeMsgNum;
	static listBaseMsgs m_FreeBaseMessages;
	static CRITICAL_SECTION m_CSFreeMsg;

	//保存消息的相关优先级数据
    typedef std::map<long, short>	mapMsgPrioLvlParams;
    typedef std::map<long, short>::iterator itMsgPrio;
	//保存部分消息的优先级级别
	static mapMsgPrioLvlParams m_MsgPrioLvl;

    typedef std::set<long>	setDiscardMsgs;
    typedef std::set<long>::iterator itDiscaMsg;
	//保存可以丢弃的消息类型
	static setDiscardMsgs m_DiscardMsgs;
public:
	virtual void SetSocketID(long lSocketID) = 0;
	virtual long GetSocketID()=0;

	virtual void SetIP(uint32_t dwIP)=0;
	virtual uint32_t GetIP()=0;

	virtual void SetIP(const char* pszIP){};

	void SetNetFlag(long lFlag)	{m_lNetFlag = lFlag;}
	long GetNetFlag()			{return m_lNetFlag;}

	//设置消息的优先级数据
	void SetPriorityLvl(long lPrioLvl)	{m_lPriorityLvl=lPrioLvl;}
	void SetStartSendTime(uint32_t dwSendTime) {m_dwStartSendTime=dwSendTime;}
	//得到计算出优先级别值
	long GetPriorityValue(uint32_t dwCurTime);
	//判断两个消息的丢弃标记是否一样
	virtual bool IsDiscardFlagEqual(CBaseMessage* pMsg) {return false;}

	static CDataBlockAllocator*	m_pDBAllocator;
    static std::set<CBaseMessage*>	TestMsg;

	static bool Initial(CDataBlockAllocator* pDBAllocator,long nMaxFreeMsgNum);
	static bool Release();

	static void RegisterMsgPrio(long lMsgType,short nPrio);
	static short GetMsgPrio(long lMsgType);

	static void RegisterDiscardMsg(long lMsgType);
	//得到该消息类型是否可以丢弃
	static bool GetIsDiscard(long lMsgType);

	//申请具体消息的函数
	typedef CBaseMessage*(*NEWMSG)();
	static NEWMSG NewMsg; 

	static CBaseMessage* __stdcall AllocMsg();
	static void __stdcall FreeMsg(CBaseMessage* pMsg);

	////////////////////////////////////////////////////////////////////////
	//	消息处理
	////////////////////////////////////////////////////////////////////////
	virtual long DefaultRun() = 0;     //缺省的消息解析函数
	virtual long Run() = 0;            //纯虚函数，消息运行函数；

	friend class CMsgQueue;
};

