/**
* @file   DefType.h
* @date   01/06/2010
* @author Fox(yulefox.at.gmail.com)
* @brief  对部分内建类型进行定义。
*         不使用Windows内建类型：包括但不限于bool、BYTE、WORD、DWORD；
*         不建议使用unsigned类型；
*         buffer可直接使用byte。
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
