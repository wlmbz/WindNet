//=============================================================================
/**
 *  file: DataBlock.cpp
 *
 *  Brief:�й̶���С���ڴ�죬��̬�������ݿ�Ļ���������
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
    CHECK(NULL != m_pBase) << "�ں���CDataBlock::CDataBlock(...),�����ڴ����";
}

CDataBlock::~CDataBlock(void)
{
	SAFE_DELETE_ARRAY(m_pBase);
}

void CDataBlock::SetCurSize(long lSize)
{
	m_nCurSize = (lSize > 0 && lSize <= m_nMaxSize) ? lSize : 0;
}
