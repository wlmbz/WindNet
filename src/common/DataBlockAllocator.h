//=============================================================================
/**
 *  file: DataBlockAllocator.h
 *
 *  Brief:�̶���С���ڴ�������
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
	//��ǰ��д���ݿ���
	uint32_t	nCurNum;
	//��ǰ��д���ݿ�Ĵ�С
	uint32_t	nCurDBSize;
	//��ǰ��д���ݵ�λ��
	uint32_t	nCurPos;
	//��ǰ��д���ݿ��ָ��
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
	//�����߼�����ʱʹ�õ����ݿ�
	//�������࣬ʹ�ýϴ�����ݿ���������ִ�С������
    typedef std::multimap<long, byte*>	MMapShareDB;
    typedef std::pair <long, byte*> ShareDBPair;
    typedef std::multimap<long, byte*>::iterator itShareDB;
	MMapShareDB m_ShareDataBlocks;

    typedef std::map<byte*, long>	mapAllocedRecord;
    typedef std::map<byte*, long>::iterator itAllocR;
	mapAllocedRecord	m_AllocedShareDB;
	//�������������ݿ����
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


