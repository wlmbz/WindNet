// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <memory>
#include "Thread.h"


class CServer;

class CServerWorker : public CThread
{
public:
    explicit CServerWorker(CServer* pServer);
    ~CServerWorker();

private:
    int Run() override;

private:
    CServer* server_ = nullptr;
};

typedef std::shared_ptr<CServerWorker>  ServerWorkerPtr;
