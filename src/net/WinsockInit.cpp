// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "WinsockInit.h"
#include <WinSock2.h>
#include "common/Logging.h"


WinsockInit::WinsockInit()
{
    WSADATA data;
    int r = WSAStartup(MAKEWORD(2, 2), &data);
    CHECK(r == 0);
}

WinsockInit::~WinsockInit()
{
    WSACleanup();
}


namespace {

// Static variable to ensure that winsock is initialised
static const WinsockInit  init_instance;

}
