
//=============================================================================
/**
 *  file: DataBlockWriteRead.h
 *
 *  Brief:������һ�����ݿ��д����
 *
 *  Authtor:wangqiao
 *	
 *	Date:2008-11-21
 */
//=============================================================================

#pragma once

#include <vector>
#include "common/DefType.h"
#include "common/GUID.h"

class CDataBlock;
class CDataBlockAllocator;
class CGUID;
struct tagDBOpParam;

//������һ��д���ݿ�ļ���
typedef struct tagDataBlockWriteSet
{
	typedef std::vector<CDataBlock*>::iterator itDataSet;
    std::vector<CDataBlock*>* pDataSet;

	//���ݿ������
	CDataBlockAllocator* pDBAllocator;
	//��ǰ���ݿ鼯�ϲ�������
	tagDBOpParam* pDBWriteParam;
	uint32_t *pToltalSize;

	tagDataBlockWriteSet()
		:pDBAllocator(NULL)
		,pDBWriteParam(NULL)
		,pDataSet(NULL)
		,pToltalSize(NULL)
	{
	}

	void Initial(CDataBlockAllocator* pAllocator,
							tagDBOpParam* pWriteParam,
							std::vector<CDataBlock*>* pSet,
							uint32_t* pSize)
	{
		pDBAllocator = pAllocator;
		pDBWriteParam = pWriteParam;
		pDataSet = pSet;
		pToltalSize =pSize;
	}

	void AddToByteArray(void* pSource, long n);	// �������
	void AddToByteArray(const char* pStr);
	void AddToByteArray(const CGUID& guid);
	inline void AddToByteArray(long l)
	{
		AddBuff((byte*)&l,sizeof(long));
	}
	inline void AddToByteArray(short l)
	{
		AddBuff((byte*)&l,sizeof(short));
	}
	inline void AddToByteArray(char l)
	{
		AddBuff((byte*)&l,sizeof(char));
	}
	inline void AddToByteArray(float l)
	{
		AddBuff((byte*)&l,sizeof(float));
	}
	inline void AddToByteArray(double l)
	{
		AddBuff((byte*)&l,sizeof(double));
	}
	inline void AddToByteArray(byte l)
	{
		AddBuff((byte*)&l,sizeof(byte));
	}
	inline void AddToByteArray(uint16_t l)
	{
		AddBuff((byte*)&l,sizeof(uint16_t));
	}
	inline void AddToByteArray(uint32_t l)
	{
		AddBuff((byte*)&l,sizeof(uint32_t));
	}
private:
	void AddBuff(byte* pBuf, long size);
	void AddWrDataBlock();
}DBWriteSet;


typedef struct tagDataBlockReadSet
{
	typedef std::vector<CDataBlock*>::iterator itDataSet;
    std::vector<CDataBlock*>* pDataSet;

	//��ǰ���ݿ鼯�ϲ�������
	tagDBOpParam* pDBReadParam;

	tagDataBlockReadSet()
		:pDBReadParam(NULL)
		,pDataSet(NULL)
	{
	}
	void Initial(tagDBOpParam* pReadParam,
        std::vector<CDataBlock*>* pSet)
	{
		pDBReadParam = pReadParam;
		pDataSet = pSet;
	}
	
	void* GetBufferFromByteArray(void* pBuf, long lLen);
	char* GetStringFromByteArray(char* pStr, long lMaxLen);
	bool  GetBufferFromByteArray(CGUID& guid);

	// ��CByteArray��ȡlong
	inline long GetLongFromByteArray()
	{
		long l = 0;
		GetBuff((byte*)&l,sizeof(long));
		return l;
	}

	// ��CByteArray��ȡshort
	inline short GetShortFromByteArray()
	{
		short l = 0;
		GetBuff((byte*)&l,sizeof(short));
		return l;
	}

	// ��CByteArray��ȡchar
	inline char GetCharFromByteArray()
	{
		char l  = 0;
		GetBuff((byte*)&l,sizeof(char));
		return l;
	}

	inline float GetFloatFromByteArray()
	{
		float l = 0;
		GetBuff((byte*)&l,sizeof(float));
		return l;
	}

	inline double GetDoubleFromByteArray()
	{
		double l = 0;
		GetBuff((byte*)&l,sizeof(double));
		return l;
	}
	
	inline byte GetByteFromByteArray()
	{
		byte l = 0;
		GetBuff((byte*)&l,sizeof(byte));
		return l;
	}

	inline uint16_t GetWordFromByteArray()
	{
		uint16_t l= 0;
		GetBuff((byte*)&l,sizeof(uint16_t));
		return l;
	}

	inline uint32_t GetDwordFromByteArray()
	{
		uint32_t l = 0;
		GetBuff((byte*)&l,sizeof(uint32_t));
		return l;
	}

private:
	void* GetBuff(byte* pByte, long size);
	void AddRdDataBlock();

}DBReadSet;
