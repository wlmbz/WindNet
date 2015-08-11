/**
 * @file Public/Nets/BaseMessage.h
 *
 * @brief ��Ϸ����Ϣ���ӱ�����������ÿ����Ϣ�Ļ������ݣ���С�����ͺ�ʱ�䣩
 * �ṩ��������װ��Ϣ��������Ϣ����
 *
 * @authtor wangqiao
 * @date 2005-4-15
 * Modify 2007-4-13,�����˴���ͽṹ���Ż���Ч��
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

//������Ϣ��������ȼ�
const int MAX_MSG_PRIO = 100;
const int HEAD_SIZE = 4;

enum eBaseMsgType
{
	BASEMSG_Socket_Accept = 1,
	BASEMSG_Socket_Close,
	BASEMSG_Transfer_Congestion,//����ӵ��
	BASEMSG_Transfer_Smoothly,//���䳩ͨ
};

class CBaseMessage
{
public:

	CBaseMessage();
	virtual ~CBaseMessage();

	struct stMsg
	{
		uint32_t lSize;		// ��Ϣ��С
		uint32_t eType;		// ��Ϣ����
		uint32_t lVerifyCode;	// У����
	};

	////////////////////////////////////////////////////////////////////////
	//	��������
	////////////////////////////////////////////////////////////////////////
private:

	//��Ϣ�����ݲ���
	typedef std::vector<CDataBlock*>::iterator itMsgData;
    std::vector<CDataBlock*> m_MsgData;
	
	//�����ݴ�С
	
	tagDBOpParam m_ReadParam;
	////��ǰ�����ݿ���
	//long	m_nCurRdNum;
	////��ǰ�����ݿ�Ĵ�С
	//long	m_nCurRdDBSize;
	////��ǰ�����ݵ�λ��
	//long	m_nCurRdPos;
	////��ǰ�����ݿ��ָ��
	//byte	*m_pRdDBPtr;

	tagDBOpParam m_WriteParam;
	////��ǰд���ݿ���
	//long	m_nCurWrNum;
	////��ǰд���ݿ�Ĵ�С
	//long	m_nCurWrDBSize;
	////��ǰд���ݵ�λ��
	//long	m_nCurWrPos;
	////��ǰд���ݿ��ָ��
	//byte	*m_pWrDBPtr;

	long	m_lNetFlag;
	//����Ϣ�����ȼ���
	long	m_lPriorityLvl;
	//��ʼ���͸���Ϣ���¼�
	uint32_t	m_dwStartSendTime;
	//��Ϣ�����ü���
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
	//��֤��Ϣ�����Ƿ�Ϸ�
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

	//�õ��ܴ�С(����ʵ����Ϣ��С����Ϣǰ�ĸ���ͷ)
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
	void Encrypt(const byte kn[16][6]);	/// ����Ϣ���м���
	void Decrypt(const byte kn[16][6]);	/// ����Ϣ���н���

	////////////////////////////////////////////////////////////////////////
	//	��Ӻͻ�ȡ����
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
	void AddEx(void*, long size);//���Ӱ�ȫ�ԣ���GetEx���ʹ��
	bool GetDBWriteSet(tagDataBlockWriteSet& DBWriteSet);

	virtual void Reset(void);           //�����Ϣ����

	
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
	bool m_bEncrypted;						/// ��Ϣ������ܻ��Ѽ���
	uint32_t m_lMsgTotalSize;			/// ��ֹ�����ƻ�
    typedef std::list<CBaseMessage*>	listBaseMsgs;
    typedef std::list<CBaseMessage*>::iterator itBaseMsg;
	static uint32_t m_nMaxFreeMsgNum;
	static listBaseMsgs m_FreeBaseMessages;
	static CRITICAL_SECTION m_CSFreeMsg;

	//������Ϣ��������ȼ�����
    typedef std::map<long, short>	mapMsgPrioLvlParams;
    typedef std::map<long, short>::iterator itMsgPrio;
	//���沿����Ϣ�����ȼ�����
	static mapMsgPrioLvlParams m_MsgPrioLvl;

    typedef std::set<long>	setDiscardMsgs;
    typedef std::set<long>::iterator itDiscaMsg;
	//������Զ�������Ϣ����
	static setDiscardMsgs m_DiscardMsgs;
public:
	virtual void SetSocketID(long lSocketID) = 0;
	virtual long GetSocketID()=0;

	virtual void SetIP(uint32_t dwIP)=0;
	virtual uint32_t GetIP()=0;

	virtual void SetIP(const char* pszIP){};

	void SetNetFlag(long lFlag)	{m_lNetFlag = lFlag;}
	long GetNetFlag()			{return m_lNetFlag;}

	//������Ϣ�����ȼ�����
	void SetPriorityLvl(long lPrioLvl)	{m_lPriorityLvl=lPrioLvl;}
	void SetStartSendTime(uint32_t dwSendTime) {m_dwStartSendTime=dwSendTime;}
	//�õ���������ȼ���ֵ
	long GetPriorityValue(uint32_t dwCurTime);
	//�ж�������Ϣ�Ķ�������Ƿ�һ��
	virtual bool IsDiscardFlagEqual(CBaseMessage* pMsg) {return false;}

	static CDataBlockAllocator*	m_pDBAllocator;
    static std::set<CBaseMessage*>	TestMsg;

	static bool Initial(CDataBlockAllocator* pDBAllocator,long nMaxFreeMsgNum);
	static bool Release();

	static void RegisterMsgPrio(long lMsgType,short nPrio);
	static short GetMsgPrio(long lMsgType);

	static void RegisterDiscardMsg(long lMsgType);
	//�õ�����Ϣ�����Ƿ���Զ���
	static bool GetIsDiscard(long lMsgType);

	//���������Ϣ�ĺ���
	typedef CBaseMessage*(*NEWMSG)();
	static NEWMSG NewMsg; 

	static CBaseMessage* __stdcall AllocMsg();
	static void __stdcall FreeMsg(CBaseMessage* pMsg);

	////////////////////////////////////////////////////////////////////////
	//	��Ϣ����
	////////////////////////////////////////////////////////////////////////
	virtual long DefaultRun() = 0;     //ȱʡ����Ϣ��������
	virtual long Run() = 0;            //���麯������Ϣ���к�����

	friend class CMsgQueue;
};

