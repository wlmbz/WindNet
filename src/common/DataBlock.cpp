//=============================================================================
/**
 *  file: DataBlock.cpp
 *
 *  Brief:有固定大小的内存快，动态分配数据块的基本机构。
 *
 *  Authtor:wangqiao
 *	
 *	Date:2007-6-13
 */
//=============================================================================

#include "DataBlock.h"
#include "common/logging.h"
#include "common/SafeDelete.h"

CDataBlock::CDataBlock(long maxsize)
:m_nMaxSize(maxsize)
,m_nCurSize(0)
,m_bInitData(false)
{
	m_pBase = new byte[maxsize];
    CHECK(NULL != m_pBase) << "在函数CDataBlock::CDataBlock(...),分配内存出错。";
}

CDataBlock::~CDataBlock(void)
{
	SAFE_DELETE_ARRAY(m_pBase);
}

void CDataBlock::SetCurSize(long lSize)
{
	m_nCurSize = (lSize > 0 && lSize <= m_nMaxSize) ? lSize : 0;
}
