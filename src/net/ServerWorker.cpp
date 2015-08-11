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
        //�ȴ���ɶ˿�ͨ��
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
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__, "��ɶ˿��̲߳�������,δȡ����ɰ�(bResult == false,lpPerIOData == NULL,ErrorID:%d)��",GetLastError());
            }
            else if (lpPerIOData)
            {
                uint32_t dwError = GetLastError();
                //���һ��ɾ��Client�Ĳ�������
                tagSocketOper *pSocketOper = server_->AllocSockOper();
                pSocketOper->Init(SCOT_OnError, CPDataKey, (void*)lpPerIOData, dwError);
                server_->GetSocketOperCommands().PushBack(pSocketOper);
            }
            else
            {
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"��ɶ˿��̲߳���δ֪����");
            }
        }
        else  ////data is valid,dispose!
        {
            //�˳�
            if (CPDataKey == 0)
            {
                break;
            }
            else if (lpPerIOData == NULL)
            {
                //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"��ɶ˿��̲߳�������(bResult == true,lpPerIOData == NULL)��");
            }
            else
            {
                tagSocketOper *pSocketOper = server_->AllocSockOper();
                if (lpPerIOData->OperationType == SOT_SendZeroByte)
                {
                    //����Ƿ���0�ֽڴ�С�Ĳ���
                    //���һ��ɾ��Client�Ĳ�������					
                    pSocketOper->Init(SCOT_OnSendZeroByte, CPDataKey, (void*)lpPerIOData, 0);
                }
                else if (dwNumRead == 0)
                {
                    //���һ��ɾ��Client�Ĳ�������
                    pSocketOper->Init(SCOT_OnClose, CPDataKey, (void*)lpPerIOData, 0);
                }
                else if (lpPerIOData->OperationType == SOT_Receive)
                {
                    pSocketOper->Init(SCOT_OnRecieve, CPDataKey, (void*)lpPerIOData, dwNumRead);
                }
                else if (lpPerIOData->OperationType == SOT_Send)
                {
                    //���һ����ɷ��͵���Ϣ
                    pSocketOper->Init(SCOT_OnSend, CPDataKey, (void*)lpPerIOData, dwNumRead);
                }
                server_->GetSocketOperCommands().PushBack(pSocketOper);
            }
        }
    }
    return 0;
}