/**
* @file   DefType.h
* @date   01/06/2010
* @author Fox(yulefox.at.gmail.com)
* @brief  �Բ����ڽ����ͽ��ж��塣
*         ��ʹ��Windows�ڽ����ͣ�������������bool��BYTE��WORD��DWORD��
*         ������ʹ��unsigned���ͣ�
*         buffer��ֱ��ʹ��byte��
*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>


typedef uint8_t       byte;


#define MAKE_UINT_64(a, b) \
    ((uint64_t)(((uint64_t)((uint32_t)(a))) << 32 | ((uint32_t)(b))))

#define LO_UINT_64(l)    ((uint32_t)(l))
#define HI_UINT_64(l)    ((uint32_t)(((uint64_t)(l) >> 32) & 0xFFFFFFFF))

#ifndef _QWORD_DEFINED
#define _QWORD_DEFINED
typedef __int64 QWORD, *LPQWORD;
#endif

#define MAKEQWORD(a, b)	\
    ((QWORD)(((QWORD)((DWORD)(a))) << 32 | ((DWORD)(b))))

#define LODWORD(l) \
    ((DWORD)(l))
#define HIDWORD(l) \
    ((DWORD)(((QWORD)(l) >> 32) & 0xFFFFFFFF))
