// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "Thread.h"
#include <process.h>

CThread::CThread()
{
}

CThread::~CThread()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    id_ = 0;
}

bool CThread::Start()
{
    if (handle_ == INVALID_HANDLE_VALUE)
	{
        handle_ = (HANDLE)_beginthreadex(0, 0, ThreadFunc, (void*)this, 0, &id_);
        return (handle_ == INVALID_HANDLE_VALUE);
	}
    return false;
}

bool CThread::Wait() const
{
    return Wait(INFINITE);
}

bool CThread::Wait( DWORD timeoutMillis ) const
{	
    DWORD result = WaitForSingleObject(handle_, timeoutMillis);
    if (result == WAIT_TIMEOUT)
	{
        return true;
	}
    else if (result == WAIT_OBJECT_0)
	{
        return true;
	}
	else
	{
        return false;
	}
}

bool CThread::Terminate(DWORD exitCode /* = 0 */)
{
    return (TerminateThread(handle_, exitCode) != 0);
}


unsigned int CALLBACK CThread::ThreadFunc(void* args)
{
    int r = 0;
    CThread* pThis = (CThread*)args;
    if (pThis)
    {
        try
        {
            r = pThis->Run();
        }
        catch (...)
        {
            r = -1;
        }
    }
    return r;
}
