#pragma once

#include "net/BaseMessage.h"

class CMessage : public CBaseMessage
{
public:
    explicit CMessage(long ltype);
    ~CMessage();

    int DefaultRun() override;
    int Run() override;

private:

};