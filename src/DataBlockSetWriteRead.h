
//=============================================================================
/**
 *  file: DataBlockWriteRead.h
 *
 *  Brief:定义了一个数据块读写集合
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

//定义了一个写数据块的集合
typedef struct tagDataBlockWriteSet
{
	typedef std::vector<CDataBlock*>::iterator itDataSet;
    std::vector<CDataBlock*>* pDataSet;

	//数据块分配器
	CDataBlockAllocator* pDBAllocator;
	//当前数据块集合操作参数
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

	void AddToByteArray(void* pSource, long n);	// 添加数据
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

	//当前数据块集合操作参数
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

	// 从CByteArray获取long
	inline long GetLongFromByteArray()
	{
		long l = 0;
		GetBuff((byte*)&l,sizeof(long));
		return l;
	}

	// 从CByteArray获取short
	inline short GetShortFromByteArray()
	{
		short l = 0;
		GetBuff((byte*)&l,sizeof(short));
		return l;
	}

	// 从CByteArray获取char
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
