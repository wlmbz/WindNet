// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <Windows.h>

class CIOCompletionPort
{
public:
	explicit CIOCompletionPort();
	~CIOCompletionPort();

    bool Init(DWORD concurrency = 0);

    HANDLE GetHandle() { return handle_; }
	
    bool AssociateDevice(HANDLE hDevice, ULONG_PTR completionKey);
	
    bool PostStatus(
			ULONG_PTR completionKey, 
			DWORD dwNumBytes = 0, 
			OVERLAPPED* pOverlapped = 0 );
	
	bool GetStatus(
			ULONG_PTR* pCompletionKey, 
			PDWORD pdwNumBytes,
			OVERLAPPED** ppOverlapped, 
			DWORD dwMilliseconds = INFINITE);
	
private:
    HANDLE handle_ = NULL;
};
