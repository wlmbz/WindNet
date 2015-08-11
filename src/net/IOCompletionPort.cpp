// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "IOCompletionPort.h"
#include "common/Logging.h"



CIOCompletionPort::CIOCompletionPort()
{
}

CIOCompletionPort::~CIOCompletionPort() 
{ 
    CloseHandle(handle_);
}

bool CIOCompletionPort::Init(DWORD concurrency)
{
    handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, concurrency);
    return handle_ != NULL;
}

bool CIOCompletionPort::AssociateDevice(HANDLE hDevice, ULONG_PTR completionKey)				
{
    HANDLE handle = CreateIoCompletionPort(hDevice, handle_, completionKey, 0);
    return (handle == handle_);
}

bool CIOCompletionPort::PostStatus(
				   ULONG_PTR completionKey, 
				   DWORD dwNumBytes /* = 0 */, 
				   OVERLAPPED *pOverlapped /* = 0 */) 
{
    int r = PostQueuedCompletionStatus(handle_, dwNumBytes, completionKey, pOverlapped);
    return (r != 0);
}

bool CIOCompletionPort::GetStatus(
			   ULONG_PTR *pCompletionKey, 
			   PDWORD pdwNumBytes,
			   OVERLAPPED **ppOverlapped,
               DWORD dwMilliseconds)
{
    int r = GetQueuedCompletionStatus(handle_, pdwNumBytes, pCompletionKey, ppOverlapped, dwMilliseconds);
    if (r == 0)
    {
        DWORD dwError = ::GetLastError();
        if (dwError == WAIT_TIMEOUT)
            r = 0;
    }
    return (r != 0);
}
