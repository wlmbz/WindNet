/**
* @file   DefType.h
* @date   01/06/2010
* @author Fox(yulefox.at.gmail.com)
* @brief  �Բ����ڽ����ͽ��ж��塣
*         ��ʹ��Windows�ڽ����ͣ�������������bool��BYTE��WORD��DWORD��
*         ������ʹ��unsigned���ͣ�
*         buffer��ֱ��ʹ��uchar��
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

