// Copyright (C) 2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "ServerWorker.h"
#include "Servers.h"

CServerWorker::CServerWorker(CServer* pServer)
    : server_(pServer)
{
}

CServerWorker::~CServerWorker()
{
}

int CServerWorker::Run()
{
    bool bResult;//completion port packet flag
    DWORD dwNumRead;//bytes num be readed
    ULONG_PTR CPDataKey;
    LPER_IO_OPERATION_DATA lpPerIOData;

    char strTempt[100] = "";
    while (true)
    {
        //pServer->IncWorkThreadTick();
        //等待完成端口通告
        dwNumRead = -1;
        bResult = server_->GetIOCompletionPort().GetStatus(
            &CPDataKey,
            &dwNumRead,
            (LPOVERLAPPED*)&lpPerIOData,
            INFINITE)
            ? true : false;

        if (!bResult)
        {
            if (lpPerIOData == NULL && dwNumRead == -1)
            {
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__, "完成端口线程产生错误,未取到完成包(bResult == false,lpPerIOData == NULL,ErrorID:%d)。",GetLastError());
            }
            else if (lpPerIOData)
            {
                uint32_t dwError = GetLastError();
                //添加一个删除Client的操作命令
                tagSocketOper *pSocketOper = server_->AllocSockOper();
                pSocketOper->Init(SCOT_OnError, CPDataKey, (void*)lpPerIOData, dwError);
                server_->GetSocketOperCommands().PushBack(pSocketOper);
            }
            else
            {
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"完成端口线程产生未知错误！");
            }
        }
        else  ////data is valid,dispose!
        {
            //退出
            if (CPDataKey == 0)
            {
                break;
            }
            else if (lpPerIOData == NULL)
            {
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"完成端口线程产生错误(bResult == true,lpPerIOData == NULL)！");
            }
            else
            {
                tagSocketOper *pSocketOper = server_->AllocSockOper();
                if (lpPerIOData->OperationType == SOT_SendZeroByte)
                {
                    //如果是发送0字节大小的操作
                    //添加一个删除Client的操作命令					
                    pSocketOper->Init(SCOT_OnSendZeroByte, CPDataKey, (void*)lpPerIOData, 0);
                }
                else if (dwNumRead == 0)
                {
                    //添加一个删除Client的操作命令
                    pSocketOper->Init(SCOT_OnClose, CPDataKey, (void*)lpPerIOData, 0);
                }
                else if (lpPerIOData->OperationType == SOT_Receive)
                {
                    pSocketOper->Init(SCOT_OnRecieve, CPDataKey, (void*)lpPerIOData, dwNumRead);
                }
                else if (lpPerIOData->OperationType == SOT_Send)
                {
                    //添加一个完成发送的消息
                    pSocketOper->Init(SCOT_OnSend, CPDataKey, (void*)lpPerIOData, dwNumRead);
                }
                server_->GetSocketOperCommands().PushBack(pSocketOper);
            }
        }
    }
    return 0;
}