//=============================================================================
/**
 *  file: DataBlockAllocator.h
 *
 *  Brief:固定大小的内存快分配器
 *
 *  Authtor:wangqiao
 *	
 *	Date:2007-6-13
 */
//=============================================================================


#pragma once

#include <windows.h>
#include <map>
#include <list>
#include "common/DefType.h"

class CDataBlock;
class CGUID;

struct tagDBOpParam
{
	//当前读写数据块编号
	uint32_t	nCurNum;
	//当前读写数据块的大小
	uint32_t	nCurDBSize;
	//当前读写数据的位置
	uint32_t	nCurPos;
	//当前读写数据块的指针
	byte	*pDBPtr;
};

class CDataBlockAllocator
{
public:
	CDataBlockAllocator(int nDBNum,int nDBSize,int bIniShareDB = false);
public:
	~CDataBlockAllocator(void);

private:
	typedef std::list<CDataBlock*>	FreeDataBlocks;
    typedef std::list<CDataBlock*>::iterator itFreeDB;

	FreeDataBlocks m_FreeDataBlocks;
	CRITICAL_SECTION m_CSFreeDB;
	int	m_nMaxDBNum;
	const int	m_nDBSize;
	//用在逻辑层临时使用的数据块
	//个数不多，使用较大的数据块来满足各种大小的需求
    typedef std::multimap<long, byte*>	MMapShareDB;
    typedef std::pair <long, byte*> ShareDBPair;
    typedef std::multimap<long, byte*>::iterator itShareDB;
	MMapShareDB m_ShareDataBlocks;

    typedef std::map<byte*, long>	mapAllocedRecord;
    typedef std::map<byte*, long>::iterator itAllocR;
	mapAllocedRecord	m_AllocedShareDB;
	//允许的最大共享数据块个数
    std::map<long, long>	m_MapTest;
private:
	void Initial(int bIniShareDB = false);
	void Release();

public:
	CDataBlock*	AllocDB(long lTestFlag=0);
	void FreeDB(CDataBlock*);

	byte* AllockShareDB(long lSize);
	void FreeShareDB(byte* pData);

	void PutAllocInfo(const char* pszFileName);
};


