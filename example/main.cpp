#include <iostream>
#include <thread>
#include <chrono>
#include "DataBlockAllocator.h"
#include "net/WinsockInit.h"
#include "net/Servers.h"
#include "Message.h"

enum eNetFlag
{
    NF_Server_Client = 0x01,
    NF_Server_World = 0x10,
    NF_AsscountServer_Client = 0x100000,
};

void ProcessMessage(CServer* pServer)
{
    while (true)
    {
        CMessage* pMsg = (CMessage*)pServer->GetRecvMessages()->PopMessage();
        if (pMsg)
        {
            pMsg->Run();
            CBaseMessage::FreeMsg(pMsg);
        }
        else
        {
            break;
        }
    }
}

int main(int arg, char* argv[])
{
    WinsockInit init;
    CDataBlockAllocator* pDBAllocator = new CDataBlockAllocator(1024, 1024);
    CServer* pServer = new CServer;
    pServer->Start(3, pDBAllocator);
    pServer->Host(9527, "0.0.0.0", NF_Server_World);
    for (;;)
    {
        ProcessMessage(pServer);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}