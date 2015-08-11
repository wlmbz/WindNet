#pragma once

#include <stdio.h>
#include "common/DefType.h"

class InputBitStream
{
	byte *m_pData;

	int m_byMask;
	int m_iRack;
	
	int m_iPosition;
    int m_iSize;

public:
	InputBitStream(void);
	~InputBitStream(void);

	void LoadFromFile(FILE *fp,int iSize);

	int InputBit();
	unsigned int InputBits(int iCount);
};