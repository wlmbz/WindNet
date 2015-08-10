/**
* @file   DefType.h
* @date   01/06/2010
* @author Fox(yulefox.at.gmail.com)
* @brief  对部分内建类型进行定义。
*         不使用Windows内建类型：包括但不限于bool、BYTE、WORD、DWORD；
*         不建议使用unsigned类型；
*         buffer可直接使用uchar。
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned char       byte;
typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ulonglong;


#define MAKE_UINT_64(a, b) \
    ((ulonglong)(((ulonglong)((ulong)(a))) << 32 | ((ulong)(b))))

#define LO_UINT_64(l)    ((ulong)(l))
#define HI_UINT_64(l)    ((ulong)(((ulonglong)(l) >> 32) & 0xFFFFFFFF))

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

