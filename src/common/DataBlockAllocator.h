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
#include "base/DefType.h"

class CDataBlock;
class CGUID;

struct tagDBOpParam
{
	//��ǰ��д���ݿ���
	ulong	nCurNum;
	//��ǰ��д���ݿ�Ĵ�С
	ulong	nCurDBSize;
	//��ǰ��д���ݵ�λ��
	ulong	nCurPos;
	//��ǰ��д���ݿ��ָ��
	uchar	*pDBPtr;
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
    typedef std::multimap<long, uchar*>	MMapShareDB;
    typedef std::pair <long, uchar*> ShareDBPair;
    typedef std::multimap<long, uchar*>::iterator itShareDB;
	MMapShareDB m_ShareDataBlocks;

    typedef std::map<uchar*, long>	mapAllocedRecord;
    typedef std::map<uchar*, long>::iterator itAllocR;
	mapAllocedRecord	m_AllocedShareDB;
	//�������������ݿ����
    std::map<long, long>	m_MapTest;
private:
	void Initial(int bIniShareDB = false);
	void Release();

public:
	CDataBlock*	AllocDB(long lTestFlag=0);
	void FreeDB(CDataBlock*);

	uchar* AllockShareDB(long lSize);
	void FreeShareDB(uchar* pData);

	void PutAllocInfo(const char* pszFileName);
};


