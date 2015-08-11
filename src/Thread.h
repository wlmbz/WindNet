// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <Windows.h>
#include <cstdint>


class CThread 
{
public:
	CThread();	
	virtual ~CThread();

    CThread(const CThread&) = delete;
    CThread& operator = (const CThread&) = delete;
	
    HANDLE GetHandle() const { return handle_; }
    uint32_t  GetID() const;

	bool Wait() const;
	bool Wait(DWORD dwMillsec) const;
	
	bool Start();	
	bool Terminate(DWORD exitCode = 0);
	
private:
	virtual int Run() = 0;
    static unsigned int __stdcall ThreadFunc(void* args);
	
    uint32_t    id_ = 0;
    HANDLE      handle_ = INVALID_HANDLE_VALUE;
};
