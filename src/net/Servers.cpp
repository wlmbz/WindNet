//=============================================================================
/**
*  file: Servers.cpp
*
*  Brief:主要是服务器端使用的网络封装，用IOCP方式实现的网络接口
*			提供了网络监听和主动连接功能
*
*  Authtor:wangqiao
*	
*	Datae:2005-4-15
*
*	Modify:2007-4-13,整理了代码和结构，优化了效率
*/
//=============================================================================


#include "Servers.h"
#include <time.h>
#include <mmsystem.h>
#include <process.h>
#include "serverClient.h"
#include "BaseMessage.h"
#include "common/SafeDelete.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CServer::CServer()
{
    m_bAcceptThreadExit=false;
    m_bWorkerThreadsExit=false;

    m_lCurClientCount = 0;

    m_hNetMainTheads=0;
    m_hAcceptThread = 0;	
    m_hWorkerThreads.clear();

    m_bCheck = false;
    m_lForbidTime = 0;	
    m_ForbidIPs.clear();
    m_lMaxBlockConnetNum = SOMAXCONN;
    m_lSendInterTime = 80000;


    m_lMaxClientConNum = 100;			//最大的客户端连接数量

    m_lPendingWrBufNum = 4096;			//在每个客户端最大的发送操作数量
    m_lPendingRdBufNum = 2048;
    m_lMaxSendSizePerSecond = 8192;		//限制的每秒钟发送大小
    m_lMaxRecvSizePerSecond = 4096;		//限制的每秒钟接受大小 
    m_lMaxBlockSendMsgNum = 1024;

    m_lConPendingWrBufNum = 4096;		//在每个连接端最大的发送操作数量
    m_lConPendingRdBufNum = 2048;
    m_lConMaxSendSizePerSecond = 81920;	//连接端限制每秒钟发送数据的大小
    m_lConMaxRecvSizePerSecond = 40960;	//连接端限制每秒钟接受数据的大小
    m_lConMaxBlockSendMsgNum = 1024;

    m_lIOOperDataBufNum = 2;

    m_dwWorkThreadTick=0;			//完成端口工作线程心跳计数
    m_dwNetThreadTick=0;			//网络线程心跳计数
    m_dwAcceptThreadTick=0;		//监听接受线程心跳计数

    m_NetDataStats.clear();
    m_SendMsgsStat.clear();
    m_RecvMsgsStat.clear();
}

CServer::~CServer()
{	
    CloseHandle( m_hAcceptThread );
    CloseHandle(m_hNetMainTheads);
}

//启动网络服务器
//bMode启动模式,1:以客户端的方式启动,2:以服务器的方式启动
bool	CServer::Start(BYTE bMode,CDataBlockAllocator* pDBAllocator,
                       long nMaxFreeSockOperNum,long nMaxFreeIOOperNum,long lIOOperDataBufNum,
                       bool bCheck,long lForbidTime,
                       long lMaxMsgLen,long lMaxConNums,long lTotalClientsNum,
                       long lPendingWrBufNum,long lPendingRdBufNum,
                       long lMaxSendSizePerSecond,long lMaxRecvSizePerSecond,
                       long lMaxBlockSendMsgNum,
                       long lConPendingWrBufNum,long lConPendingRdBufNum,
                       long lConMaxSendSizePerSecond,long lConMaxRecvSizePerSecond,
                       long lConMaxBlockSendMsgNum)
{
    m_bMode = bMode;
    m_pDBAllocator=pDBAllocator;
    m_nMaxFreeSockOperNum = nMaxFreeSockOperNum;
    m_nMaxFreeIOOperNum = nMaxFreeIOOperNum;
    m_lIOOperDataBufNum = lIOOperDataBufNum;

    m_bCheck = bCheck;
    m_lForbidTime = lForbidTime;m_lMaxMessageLength=lMaxMsgLen;
    m_lMaxClientConNum = lMaxConNums;m_lTotalClientsNum = lTotalClientsNum;
    m_lPendingWrBufNum = lPendingWrBufNum;m_lPendingRdBufNum=lPendingRdBufNum;
    m_lMaxSendSizePerSecond = lMaxSendSizePerSecond;m_lMaxRecvSizePerSecond = lMaxRecvSizePerSecond;
    m_lMaxBlockSendMsgNum = lMaxBlockSendMsgNum;
    m_lConPendingWrBufNum = lConPendingWrBufNum;m_lConPendingRdBufNum=lConPendingRdBufNum;
    m_lConMaxSendSizePerSecond= lConMaxSendSizePerSecond;m_lConMaxRecvSizePerSecond = lConMaxRecvSizePerSecond;
    m_lConMaxBlockSendMsgNum = lConMaxBlockSendMsgNum;
    //预分配命令操作结构
    InitializeCriticalSection(&m_CSSockOper);
    InitializeCriticalSection(&m_CSMsgStat);

    uint32_t i = 0;
    for(;i<m_nMaxFreeSockOperNum;i++)
    {
        tagSocketOper* pSockOper = new tagSocketOper;
        m_FreeSocketOpers.push_back(pSockOper);
    }
    //预分配IO操作结构
    InitializeCriticalSection(&m_CSIOOper);
    i = 0;
    for(;i<m_nMaxFreeIOOperNum;i++)
    {
        PER_IO_OPERATION_DATA* pPerIOData  = new PER_IO_OPERATION_DATA(m_lIOOperDataBufNum);
        m_FreeListIOOpers.push_back(pPerIOData);
    }

    //初始化预分配的客户端
    InitClients();


    //得到CPU的数量
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    //根据cpu数量创建完成端口
    if (!completion_port_.Init(sysInfo.dwNumberOfProcessors))
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"创建完成端口失败!");
        return false;
    }

    //创建网络主线程
    bool bRet = CreateNetMainThread();
    if(!bRet)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"创建网络主线程失败!");
        return false;
    }

    //创建在完成端口上等待的工作者线程
    bRet=CreateWorkerThreads(sysInfo.dwNumberOfProcessors);	
    if(!bRet)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"创建工作线程失败!");
        return false;
    }
    //客户端方式启动
    if(bMode&0x1)
    {
    }
    //服务器方式启动
    if(bMode&0x2)
    {
        //Host(dwPort,strIP,nSocketType);
    }

    //设置本机IP
    SetHostName();
    return true;
}

// 创建 Listen Socket，进入监听状态，等待客户端的连接
bool CServer::Host(UINT dwPort, const char* strIP,long lFlag, uint32_t nSocketType)
{
    if(!(m_bMode&2))	return FALSE;

    SetFlag(lFlag);

    // 创建 socket
    bool bRet = CreateEx(dwPort, strIP,nSocketType);	
    //创建失败
    if(!bRet)
    {
        return bRet;
    }

    //创建接收客户socket线程
    bRet=CreateAcceptThread();
    if(!bRet)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"创建接受连接线程失败!");

        return FALSE;
    }

    // listen
    bRet = Listen(m_lMaxBlockConnetNum);
    if(!bRet)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"listen 失败!");
        return FALSE;
    }
    return bRet;
}

void CServer::InitClients()
{
    InitializeCriticalSection(&m_CSClient);
    //预分配连接客户端
    m_pClients = new CServerClient*[m_lTotalClientsNum];
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        CServerClient *pClient = new CServerClient(this,m_pDBAllocator);
        m_pClients[i] = pClient;
    }
}

void CServer::ReleaseClients()
{
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        CServerClient *pClient =m_pClients[i];
        SAFE_DELETE(pClient);
    }

    delete []m_pClients;
    m_lTotalClientsNum = 0;

    DeleteCriticalSection(&m_CSClient);
}

//创建网络主线程
bool CServer::CreateNetMainThread()
{
    g_NewAcceptSockets.clear();
    unsigned threadID;
    m_hNetMainTheads = (HANDLE)_beginthreadex(NULL,0,NetThreadFunc,this,0,&threadID);

    if(m_hNetMainTheads==NULL)
        return FALSE;
    return TRUE;
}
//创建在完成端口上等待的工作者线程
bool CServer::CreateWorkerThreads(int concurrenty)
{
    for (int i = 0; i<concurrenty; i++)
    {
        ServerWorkerPtr worker(new CServerWorker(this));
        m_hWorkerThreads.push_back(worker);
    }

    //PutTraceString(NET_MODULE,"服务器创建%d个完成端口工作线程。",nProcNum);
    return TRUE;
}


//创建接受客户socket线程
bool CServer::CreateAcceptThread()
{
    unsigned threadID;
    m_hAcceptThread = (HANDLE)_beginthreadex(NULL,0,AcceptThreadFunc,this,0,&threadID);

    if(m_hAcceptThread==NULL)
        return FALSE;
    return TRUE;
}

//退出服务端套接字
void CServer::Exit()
{
    ExitWorkerThread();

    //如果有客户端模式,删除分配列表
    if(m_bMode&0x1)
    {
    }

    itSockOP itSock = m_FreeSocketOpers.begin();
    for(;itSock != m_FreeSocketOpers.end();itSock++)
    {
        SAFE_DELETE((*itSock));
    }
    DeleteCriticalSection(&m_CSSockOper);
    DeleteCriticalSection(&m_CSMsgStat);

    itIOOper itIo = m_FreeListIOOpers.begin();
    for(;itIo != m_FreeListIOOpers.end();itIo++)
    {
        SAFE_DELETE((*itIo));
    }
    DeleteCriticalSection(&m_CSIOOper);

    //释放Serveclient变量
    ReleaseClients();

    //释放消息队列里的消息
    CBaseMessage* pMsg = NULL;
    while(pMsg = GetRecvMessages()->PopMessage())
    {
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
}

//设置退出完成端口工作者线程标志
void CServer::ExitWorkerThread(void)
{
    //接受线程退出
    //m_bAcceptThreadExit=true;
    //WaitForSingleObject(m_hAcceptThread,INFINITE);
    //如果是服务器模式，关闭连接线程
    if(m_bMode&0x2)
    {
        TerminateThread(m_hAcceptThread,0);
    }

    CloseAll();

    while(m_lCurClientCount>0)
    {
        Sleep(100);
    }


    //完成端口的工作线程退出
    for (int i = 0; i < m_hWorkerThreads.size(); i++)
    {
        //给工作者线程发送退出消息
        completion_port_.PostStatus(0);
        m_hWorkerThreads[i]->Wait();
    }	

    //发送退出主线程操作命令
    tagSocketOper* pSocketOpera = AllocSockOper();
    pSocketOpera->Init(SCOT_ExitThread,0,0);
    m_SocketOperaCommands.PushBack(pSocketOpera);
    //等候网络主线程完成
    WaitForSingleObject(m_hNetMainTheads,INFINITE);
}


tagSocketOper* CServer::AllocSockOper()
{
    tagSocketOper* pSockOper = NULL;

    EnterCriticalSection(&m_CSSockOper);
    if(m_FreeSocketOpers.size() > 0)
    {
        pSockOper = m_FreeSocketOpers.front();
        m_FreeSocketOpers.pop_front();
    }
    LeaveCriticalSection(&m_CSSockOper);

    if(pSockOper == NULL)
        pSockOper = new tagSocketOper();
    return pSockOper;
}


void CServer::FreeSockOper(tagSocketOper* pSockOper)
{
    if(pSockOper == NULL)	return;
    EnterCriticalSection(&m_CSSockOper);
    if(m_FreeSocketOpers.size() < m_nMaxFreeSockOperNum)
    {
        m_FreeSocketOpers.push_back(pSockOper);
    }
    else
    {
        SAFE_DELETE(pSockOper);
    }
    LeaveCriticalSection(&m_CSSockOper);
}

PER_IO_OPERATION_DATA* CServer::AllocIoOper()
{
    PER_IO_OPERATION_DATA* pPerIOData = NULL;
    EnterCriticalSection(&m_CSIOOper);
    if(m_FreeListIOOpers.size() > 0)
    {
        pPerIOData = m_FreeListIOOpers.front();
        m_FreeListIOOpers.pop_front();
    }
    LeaveCriticalSection(&m_CSIOOper);

    if(NULL == pPerIOData)
        pPerIOData = new PER_IO_OPERATION_DATA(m_lIOOperDataBufNum);
    return pPerIOData;
}

void CServer::FreeIoOper(PER_IO_OPERATION_DATA* pPerIOData)
{
    if(pPerIOData == NULL)	return;

    pPerIOData->pParam = NULL;
    EnterCriticalSection(&m_CSIOOper);
    if(m_FreeListIOOpers.size() < m_nMaxFreeIOOperNum)
    {
        if(m_lIOOperDataBufNum > pPerIOData->m_lDataBufNum)
            pPerIOData->ResetDataBufSize(m_lIOOperDataBufNum);
        m_FreeListIOOpers.push_back(pPerIOData);
    }
    else
    {
        SAFE_DELETE(pPerIOData);
    }
    LeaveCriticalSection(&m_CSIOOper);
}

// [TCP] 处理服务器关闭的请求
void CServer::OnClose(int nErrorCode)
{
    CMySocket::OnClose(nErrorCode);
}

//Server工作在完成端口模式下时，使用该方法代替OnAccept
void CServer::OnAccept(int nErrorCode)
{

    char strInfo[256]="";
    SOCKET clientSocket;
    struct sockaddr_in stClientAddress;
    int nClientAddressLen=sizeof(stClientAddress);    

    clientSocket = WSAAccept (m_hSocket,
        (struct sockaddr*)&stClientAddress,
        &nClientAddressLen,
        NULL,
        0);

    if(clientSocket==INVALID_SOCKET)
    {
        uint32_t dwEorror = WSAGetLastError();
        return;
    }

    //判断是否超过最大连接数
    if(m_lCurClientCount >= m_lMaxClientConNum)
    {
        closesocket(clientSocket);
        //PutTraceString(NET_MODULE,"the cur connect num(%d) greater the restrict num(%d)",m_lCurClientCount,m_lMaxClientConNum);

        return;
    }

    //察看此IP是否被禁止
    if(m_bCheck && FindForbidIP(stClientAddress.sin_addr.S_un.S_addr) == true)
    {
        closesocket(clientSocket);
        //Log4c::Warn(NET_MODULE,"the ip address(%d) has already forbided",stClientAddress.sin_addr.S_un.S_addr);
        return;
    }

    //生成
    CServerClient* pServerClient=  AllocClient(true);
    if(NULL == pServerClient)
    {
        //Log4c::Warn(NET_MODULE,"erro, Don't allocate the 'serverclient' instance.！");
        closesocket(clientSocket);
        return;
    }

    pServerClient->m_hSocket=clientSocket;
    pServerClient->SetFlag(GetFlag());
    /// 设置加解密所用Key.
    static bool bFirst = true;
    if (IsEncryptType(pServerClient->GetFlag()))
    {
        if (bFirst)
        {
            bFirst = false;
            struct sockaddr_in addr;
            int len = sizeof(addr);
            getsockname(clientSocket, (struct sockaddr *)&addr, &len);
            SetKey(addr.sin_addr.S_un.S_addr, addr.sin_port);
        }
        pServerClient->GetKn(m_kn);
    }

    //设置IP
    pServerClient->SetPeerIP(inet_ntoa(stClientAddress.sin_addr));
    pServerClient->SetPeerIP(stClientAddress.sin_addr.S_un.S_addr);
    pServerClient->SetPeerPort(stClientAddress.sin_port);
    pServerClient->SetParam(GetCheck(),GetMaxMsgLen(),m_lMaxBlockSendMsgNum,
        m_lPendingWrBufNum,m_lPendingRdBufNum,
        m_lMaxSendSizePerSecond,m_lMaxRecvSizePerSecond,0);
    pServerClient->SetClose(false);
    pServerClient->SetShutDown(false);

    //设置Socket的indexID
    pServerClient->SetSendRevBuf();
    //设置套接字选项
    pServerClient->SetSocketOpt();

    tagSocketOper *pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_Init,pServerClient->GetIndexID(),pServerClient);
    m_SocketOperaCommands.PushBack(pSocketOper);

    pServerClient->OnAccept();

    IncAcceptThreadTick();
}

// error的时候调用
void CServer::OnError(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,int errorCode)
{
    if(NULL == pPerIOData)	return;
    char str[200];
    if(pPerIOData->OperationType == SOT_Send)
    {
        sprintf(str, "完成端口线程产生一次失败的发送IO操作(ErrorID:%d)。",errorCode);
        CBaseMessage* pMsg = (CBaseMessage*)pPerIOData->pParam;
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
    else if(pPerIOData->OperationType == SOT_Receive)
    {
        sprintf(str, "完成端口线程产生一次失败的接受IO操作(ErrorID:%d)。",errorCode);
        m_pDBAllocator->FreeDB((CDataBlock*)pPerIOData->pParam);
    }


    CServerClient *pClient = FindClient(lIndexID);
    if(pClient)
    {
        //取消超时监视
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(pClient->GetIndexID());
        pClient->OnClose();
        FreeClient(pClient);
    }
    FreeIoOper(pPerIOData);

    if(errorCode != 64)
    {
       //PutTraceString(NET_MODULE,str);
    }
}

//当关闭的时候
void CServer::OnClose(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID)
{
    if(NULL == pPerIOData)	return;

    if(pPerIOData->OperationType == SOT_Send)
    {
        CBaseMessage* pMsg = (CBaseMessage*)pPerIOData->pParam;
        if(pMsg->RemoveRefCount() == 0)
            CBaseMessage::FreeMsg(pMsg);
    }
    else if(pPerIOData->OperationType == SOT_Receive)
    {
        m_pDBAllocator->FreeDB((CDataBlock*)pPerIOData->pParam);
    }

    CServerClient *pClient = FindClient(lIndexID);
    if(pClient)
    {
        //取消超时监视
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(pClient->GetIndexID());
        pClient->OnClose();
        FreeClient(pClient);
    }
    FreeIoOper(pPerIOData);
}

//在完成端口上发送数据结束
void CServer::OnSend(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lSendNum)
{
    if(NULL == pPerIOData)	return;

    CBaseMessage *pMsg = (CBaseMessage*)pPerIOData->pParam;
    //long lMsgType = pMsg->GetType();
    //long lMsgSize = pMsg->GetTotalSize();	
    if(pMsg->RemoveRefCount() == 0)
        CBaseMessage::FreeMsg(pMsg);
    FreeIoOper(pPerIOData);

    CServerClient* pClient = FindClient(lIndexID);
    long lFlag = -1;
    if(pClient)
    {
        lFlag = pClient->GetFlag();
        pClient->DecPendingWrBufNum(lSendNum);
        //继续尝试发送消息
        pClient->Send(NULL,0,0);

        //pClient->OnTransferChange();
    }
}


void CServer::OnSendZeorByte(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID)
{
    if(NULL == pPerIOData)	return;
    FreeIoOper(pPerIOData);
    CServerClient* pClient = FindClient(lIndexID);
    if(pClient)
    {
        pClient->OnSendZeroByteData();
        //继续尝试发送消息
        pClient->Send(NULL,0,0);
    }
}

void CServer::OnDisConn(long lIndexID)
{
    CServerClient* pClient = FindClient(lIndexID);
    if(pClient)
    {
        pClient->ShutDown();
    }	
}

void CServer::OnReceive(PER_IO_OPERATION_DATA *pPerIOData,long lIndexID,long lReadNum)
{
    CServerClient* pClient = FindClient(lIndexID);	

    if( pClient )
    {
        if(pClient->IsAccept())
            RemoveNewAcceptSocket(lIndexID);
        CDataBlock *pReadDB = (CDataBlock*)pPerIOData->pParam;

        //添加读数据的大小
        pClient->AddRecvSize(lReadNum);
        //设置读取数据的大小
        pReadDB->SetCurSize(lReadNum);
        //添加完成的读数据
        pClient->AddReceiveData(pPerIOData->m_nSequenceNumber,pReadDB);
        //重新开始读取数据
        if(pClient->ReadFromCompletionPort(pPerIOData,NULL) )
        {
            pClient->OnReceive();
            //如果平均每秒接受的大小超过限制值,发出错误信息.断开
            if(pClient->GetCurRecvSizePerSecond() > pClient->GetMaxRecvSizePerSecond() )
            {				
                //Log4c::Warn(NET_MODULE,"erro,(SocketID:%d) the current rcv data size per-second(%d) greater the max value(%d),shutdown the socket!",
                //    lIndexID,pClient->GetCurRecvSizePerSecond(),pClient->GetMaxRecvSizePerSecond());
                //关闭
                pClient->Close();;
            }
        }		
        else
        {
            //处理错误情况
            FreeIoOper(pPerIOData);
            pClient->OnClose();
            FreeClient(pClient);
        }		
    }
    else
    {
        //处理错误情况
        FreeIoOper(pPerIOData);
    }
    return;
}

// [UDP] 在接收到消息时调用
void CServer::OnReceive(int nErrorCode)
{
}

//异步发送消息
int CServer::ASend(long lIndexID,CBaseMessage *pMsg)
{
    if(NULL == pMsg)	return false;
    //设置发送时间,用于计算消息的优先级
    pMsg->SetStartSendTime(timeGetTime());
    tagSocketOper *pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_Send,lIndexID,(void*)pMsg);
    m_SocketOperaCommands.PushBack(pSocketOper);
    return true;
}

// 用于邦定一个SOCKET到完成端口
bool CServer::AssociateSocketWithCP(SOCKET socket,uint32_t dwCompletionKey)
{
    if(completion_port_.GetHandle() == NULL)
    {
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"在函数 CServer::AssociateSocketWithCP(...)里操作出错，完成端口句柄无效。");
        return false;
    }
    if (!completion_port_.AssociateDevice((HANDLE)socket, dwCompletionKey))
    {
        uint32_t dwErrorID = GetLastError();
        //PutErrorString(NET_MODULE,"%-15s在函数 CServer::AssociateSocketWithCP(...)里.邦定一个Socket到完成端口出错(ErrorID:%d)",__FUNCTION__,dwErrorID);
        return false;
    }
    return true;
}

// [服务器]
bool CServer::Listen(int nConnectionBacklog)
{
    // [TCP]
    if( GetProtocol() == SOCK_STREAM )
    {
        if(SOCKET_ERROR != listen(m_hSocket, nConnectionBacklog))
        {
            return TRUE;
        }
    }
    return FALSE;
}


long CServer::OnDisConnAll()
{
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == true )
        {
            m_pClients[i]->ShutDown();
        }
    }
    LeaveCriticalSection(&m_CSClient);
    return true;
}

long CServer::OnCloseAll()
{
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == true )
        {
            m_pClients[i]->Close();
        }
    }
    //m_lCurClientCount = 0;
    LeaveCriticalSection(&m_CSClient);
    return true;
}

void	CServer::AddForbidIP(u_long laddr)
{
    m_ForbidIPs[laddr] = timeGetTime();
}

bool	CServer::FindForbidIP(u_long laddr)
{
    map<u_long,uint32_t>::iterator it = m_ForbidIPs.find(laddr);
    if(it != m_ForbidIPs.end())
    {
        uint32_t dwTime = timeGetTime();
        if(dwTime-(*it).second <= (uint32_t)m_lForbidTime)
        {
            return true;
        }
        m_ForbidIPs.erase(laddr);
    }
    return false;
}


long	CServer::AddSendSize(long lFlagID,long lSize)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it == m_NetDataStats.end() )
    {
        tagDataStat dataState;
        memset(&dataState,0,sizeof(tagDataStat));
        dataState.dwSendStartTime = dataState.dwRecvStartTime = timeGetTime();
        m_NetDataStats[lFlagID] = dataState;
        it = m_NetDataStats.find(lFlagID);
    }	
    tagDataStat &dataState = (*it).second;	
    dataState.lTotalSendSize += lSize;
    return dataState.lTotalSendSize;
}

long	CServer::AddRecvSize(long lFlagID,long lSize)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it == m_NetDataStats.end() )
    {
        tagDataStat dataState;
        memset(&dataState,0,sizeof(tagDataStat));
        dataState.dwSendStartTime = dataState.dwRecvStartTime = timeGetTime();
        m_NetDataStats[lFlagID] = dataState;
        it = m_NetDataStats.find(lFlagID);
    }
    tagDataStat &dataState = (*it).second;
    dataState.lTotalRecvSize += lSize;
    return dataState.lTotalRecvSize;
}
long	CServer::GetSendSizePerSecond(long lFlagID)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it != m_NetDataStats.end() )
    {
        tagDataStat &dataState = (*it).second;
        uint32_t dwCurTime = timeGetTime();
        uint32_t DifTime = dwCurTime-dataState.dwSendStartTime;
        if(DifTime == 0) DifTime = 1;
        long lSendSizePerSecond = dataState.lTotalSendSize*1000/DifTime;
        dataState.dwSendStartTime = dwCurTime;
        dataState.lTotalSendSize = 0;
        return lSendSizePerSecond;
    }
    return 0;
}

long	CServer::GetRecvSizePerSecond(long lFlagID)
{
    itDataStat it = m_NetDataStats.find(lFlagID);
    if(it != m_NetDataStats.end() )
    {
        tagDataStat &dataState = (*it).second;
        uint32_t dwCurTime = timeGetTime();
        uint32_t DifTime = dwCurTime-dataState.dwRecvStartTime;
        if(DifTime == 0) DifTime = 1;

        long lRecvSizePerSecond = dataState.lTotalRecvSize*1000/DifTime;
        dataState.dwRecvStartTime = dwCurTime;
        dataState.lTotalRecvSize = 0;
        return lRecvSizePerSecond;
    }
    return 0;
}


//添加发送消息数据统计
void	CServer::AddSendMsgStat(long lMsgType,long lSize)
{
    EnterCriticalSection(&m_CSMsgStat);
    itMsgStat it = m_SendMsgsStat.find(lMsgType);
    if(it == m_SendMsgsStat.end())
    {
        tagMsgStat msgStat = {0,1};
        m_SendMsgsStat.insert(make_pair(lMsgType,msgStat));
        it = m_SendMsgsStat.find(lMsgType);
    }
    tagMsgStat &msgStat = (*it).second;
    msgStat.lNum++;
    msgStat.lTotalSize += lSize;

    itMaxMsgLen itMaxLen = m_SendMsgMaxLen.find(lMsgType);
    if(itMaxLen != m_SendMsgMaxLen.end() )
    {
        if((*itMaxLen).second < lSize)
            (*itMaxLen).second = lSize;
    }
    else
        m_SendMsgMaxLen[lMsgType]=lSize;

    LeaveCriticalSection(&m_CSMsgStat);

}
//添加接受消息数据统计
void	CServer::AddRecvMsgStat(long lMsgType,long lSize)
{
    EnterCriticalSection(&m_CSMsgStat);
    itMsgStat it = m_RecvMsgsStat.find(lMsgType);
    if(it == m_RecvMsgsStat.end())
    {
        tagMsgStat msgStat = {0,1};
        m_RecvMsgsStat.insert(make_pair(lMsgType,msgStat));
        it = m_RecvMsgsStat.find(lMsgType);
    }
    tagMsgStat &msgStat = (*it).second;
    msgStat.lNum++;
    msgStat.lTotalSize += lSize;

    itMaxMsgLen itMaxLen = m_RecvMsgMaxLen.find(lMsgType);
    if(itMaxLen != m_RecvMsgMaxLen.end() )
    {
        if((*itMaxLen).second < lSize)
            (*itMaxLen).second = lSize;
    }
    else
        m_RecvMsgMaxLen[lMsgType]=lSize;

    LeaveCriticalSection(&m_CSMsgStat);
}


//输出消息统计信息到net.log文件
void	CServer::OutputMsgStatInfo(uint32_t lIntelTime)
{
    static char pszInfo[1024]="";
    static uint32_t dwLastTime = timeGetTime();	
    uint32_t dwCurTiem  = timeGetTime();
    if(dwCurTiem-dwLastTime < lIntelTime)
        return;
    dwLastTime = dwCurTiem;

    EnterCriticalSection(&m_CSMsgStat);
    //PutTraceString(NET_MODULE,"the send msgs info:");
    itMsgStat it = m_SendMsgsStat.begin();
    for(;it != m_SendMsgsStat.end();it++)
    {
       //PutTraceString(NET_MODULE,"msgType:%d,count:%d,totalsize:%d",(*it).first,(*it).second.lNum,(*it).second.lTotalSize);
    }
    m_SendMsgsStat.clear();

   //PutTraceString(NET_MODULE,"the recv msgs info:");
    it = m_RecvMsgsStat.begin();
    for(;it != m_RecvMsgsStat.end();it++)
    {
        //PutTraceString(NET_MODULE,"msgType:%d,count:%d,totalsize:%d",(*it).first,(*it).second.lNum,(*it).second.lTotalSize);
    }
    m_RecvMsgsStat.clear();

    //PutTraceString(NET_MODULE,"the msgs send max len:");
    itMaxMsgLen itMaxLen = m_SendMsgMaxLen.begin();
    for(;itMaxLen != m_SendMsgMaxLen.end();itMaxLen++)
    {
        //PutTraceString(NET_MODULE,"msgType:%d,maxLen:%d",(*itMaxLen).first,(*itMaxLen).second);
    }

    //PutTraceString(NET_MODULE,"the msgs recv max len:");
    itMaxLen = m_RecvMsgMaxLen.begin();
    for(;itMaxLen != m_RecvMsgMaxLen.end();itMaxLen++)
    {
        //PutTraceString(NET_MODULE,"msgType:%d,maxLen:%d",(*itMaxLen).first,(*itMaxLen).second);
    }

    LeaveCriticalSection(&m_CSMsgStat);
}

void	CServer::AddNewAcceptSocket(uint32_t dwSocketID)
{
    uint32_t dwTime = timeGetTime();
    g_NewAcceptSockets[dwSocketID] = dwTime;
}
void	CServer::RemoveNewAcceptSocket(uint32_t dwSocketID)
{
    g_NewAcceptSockets.erase(dwSocketID);
}
void	CServer::DoNewAcceptSocket()
{
    uint32_t dwTime = timeGetTime();
    map<uint32_t,uint32_t>::iterator it = g_NewAcceptSockets.begin();
    for(;it != g_NewAcceptSockets.end();)
    {
        if( dwTime-(*it).second >= m_lSendInterTime )
        {
            OnDisConn((*it).first );
            it = g_NewAcceptSockets.erase(it);
        }
        else
        {
            it++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

CServerClient*	CServer::AllocClient(bool bAccept)
{
    CServerClient *pClient = NULL;
    EnterCriticalSection(&m_CSClient);
    int i = 0;
    for(;i<m_lTotalClientsNum;i++)
    {
        if( m_pClients[i]->IsUsing() == false )
        {
            pClient = m_pClients[i];
            pClient->Init();

            pClient->SetUsing(true);
            pClient->SetIsAccpet(bAccept);
            pClient->IncUseCount();
            pClient->SetIndexID( i<<16|pClient->GetUseCount() );
            m_lCurClientCount++;
            break;
        }
    }
    LeaveCriticalSection(&m_CSClient);
    return pClient;
}
void CServer::FreeClient(CServerClient* pClient)
{
    if(NULL == pClient)	return;
    EnterCriticalSection(&m_CSClient);
    pClient->Release();
    pClient->SetUsing(false);
    pClient->SetTransCong(false);
    pClient->SetIndexID(0);

    LeaveCriticalSection(&m_CSClient);
    m_lCurClientCount--;
}

// 查找一个客户端，返回坐标，-1表示没有
CServerClient* CServer::FindClient(long lSocketID)
{
    long lIndex = lSocketID >> 16;
    if(lIndex >= m_lTotalClientsNum || lSocketID <= 0 )	return NULL;
    CServerClient* pClient = m_pClients[lIndex];
    if(pClient->GetIndexID() != lSocketID)	return NULL;

    return pClient;
}

// 删除一个客户端
bool CServer::FreeClient(long lSocketID)
{
    CServerClient* pClient = FindClient(lSocketID);
    FreeClient(pClient);	
    return true;	
}

//主动连接服务器,返回该连接的ID
//返回-1，表示失败
long CServer::Connect(LPCTSTR lpszHostAddress, UINT nHostPort,long lFlag,uint32_t dwTimeOut)
{
    if(!(m_bMode&1))	return -1;

    CServerClient* pConClient = AllocClient(false);
    if(pConClient == NULL)	return -1;

    if( !pConClient->CreateEx(0,NULL,SOCK_STREAM) )
    {
        //PutErrorString(NET_MODULE,"%-15s Create socket bound to %s:%u FAILED, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, GetLastError());
        return -1;
    }

    pConClient->SetFlag(lFlag);
    //设置编号
    pConClient->SetSendRevBuf();
    //设置套接字选项
    pConClient->SetSocketOpt();

    long lID = pConClient->GetIndexID();

    pConClient->SetClose(false);
    pConClient->SetShutDown(false);

    pConClient->SetParam(false,0xFFFFFFF,m_lConMaxBlockSendMsgNum,
        m_lConPendingWrBufNum,m_lConPendingRdBufNum,
        m_lConMaxSendSizePerSecond,m_lConMaxRecvSizePerSecond,lFlag);

    if( !Connect(pConClient,lpszHostAddress,nHostPort,dwTimeOut) )
    {
        pConClient->Close();
        FreeClient(pConClient);
        return -1;
    }

    tagSocketOper* pSocketOpera = AllocSockOper();
    pSocketOpera->Init(SCOT_Init,lID,pConClient,0);
    m_SocketOperaCommands.PushBack(pSocketOpera);

    return lID;
}

long CServer::Connect(CServerClient* pConClient,LPCTSTR lpszHostAddress, UINT nHostPort,uint32_t dwTimeOut)
{
    if(NULL == pConClient || NULL == pConClient->m_hSocket)	return FALSE;

    //设置连接的服务器IP和端口
    pConClient->SetPeerIP(lpszHostAddress);
    pConClient->SetPeerPort(nHostPort);

    SOCKADDR_IN sockAddr;
    memset(&sockAddr,0,sizeof(sockAddr));
    LPSTR lpszAscii = (LPTSTR)lpszHostAddress;
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
    if (sockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        //PutErrorString(NET_MODULE,"%-15s Given address %s:%u is INVALID, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, GetLastError());
        return FALSE;
    }

    sockAddr.sin_port = htons((u_short)nHostPort);
    if (IsEncryptType(pConClient->GetFlag()))
    {
        pConClient->SetKey(sockAddr.sin_addr.s_addr, sockAddr.sin_port);
    }

    int ret = connect(pConClient->m_hSocket,(SOCKADDR*)&sockAddr, sizeof(sockAddr));
    if( ret == SOCKET_ERROR )
    {
        uint32_t dwError = WSAGetLastError();
        if(  dwError != WSAEWOULDBLOCK )
        {
            return FALSE;
        }

        //等候一段时间,用是否可写来判断连接成功与否
        fd_set writefd;
        FD_ZERO(&writefd);
        FD_SET(pConClient->m_hSocket,&writefd);
        timeval waittime = {dwTimeOut/1000,(dwTimeOut%1000)*1000};
        int nRet = select(0,NULL,&writefd,NULL,&waittime);
        if(nRet == 0 || nRet == SOCKET_ERROR)
        {
            //PutErrorString(NET_MODULE,"%-15s Select %s:%u FAILED, ERR_ID: %d.",__FUNCTION__, lpszHostAddress, nHostPort, WSAGetLastError());
            return FALSE;
        }
    }
    return	TRUE;
}

//主动关闭连接
bool  CServer::DisConn(long lIndexID)
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_DisConn,lIndexID,NULL);
    m_SocketOperaCommands.PushBack(pSocketOper);
    return true;
}

bool  CServer::DisConnAll()
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_DisConnAll,0,NULL);
    m_SocketOperaCommands.PushBack(pSocketOper);
    return true;
}

bool  CServer::CloseAll()
{
    tagSocketOper* pSocketOper = AllocSockOper();
    pSocketOper->Init(SCOT_CloseAll,0,NULL);
    m_SocketOperaCommands.PushBack(pSocketOper);
    return true;
}


bool CServer::OnClientInit(CServerClient* pClient)
{
    if(pClient == NULL)	return false;

    long lIndexID = pClient->GetIndexID();


    //绑定套接字到完成端口
    if( !AssociateSocketWithCP(pClient->m_hSocket,lIndexID) )
    {
        pClient->Close();
        FreeClient(pClient);
        //PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"在函数CServer::OnConClientInitial(...)中,AssociateSocketWithCP(...)操作失败.");
        return false;
    }

    if( pClient->StartReadData() == false)
    {
        pClient->Close();
        FreeClient(pClient);
        return false;
    }

    if( pClient->IsAccept() )
        AddNewAcceptSocket(lIndexID);

    //判断接受数据是否超时，超时则关闭改套接字
    DoNewAcceptSocket();
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

unsigned __stdcall AcceptThreadFunc(void* pArguments)
{	
    CServer* pServer=(CServer*)pArguments;

    if(pServer==NULL)
    {
        _endthreadex( 0 );
        return 0;
    }

#ifndef _DEBUG
    __try
    {
#endif
        //接受client socket
        while(true)
        {	
            pServer->OnAccept(0);
        }

#ifndef _DEBUG
    }
    __except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"********服务器接受线程出错，请查看最新的错误报告文件********");

    }
#endif

    _endthreadex( 0 );
    return 0;
}

void DoNetThreadFunc(CServer* pServer)
{
    char strTempt[200]="";
    CommandsQueue TemptCommands;
    //获取一个操作命令
    bool bRun = true;
    while(bRun)
    {
        pServer->IncNetThreadTick();
        TemptCommands.clear();
        pServer->m_SocketOperaCommands.CopyAllCommand(TemptCommands);
        CommandsQueue::iterator itCommand = TemptCommands.begin();
        for(; itCommand != TemptCommands.end();itCommand++)
        {
            tagSocketOper* pSocketOper = (*itCommand);

            if(pSocketOper)
            {
                switch(pSocketOper->OperaType)
                {
                    //初始化操作
                case SCOT_Init:
                    {
                        CServerClient* pClient = (CServerClient*)(pSocketOper->pBuf);
                        pServer->OnClientInit(pClient);
                    }
                    break;
                    //主动断开某个连接
                case SCOT_DisConn:
                    {
                        pServer->OnDisConn(pSocketOper->lSocketID);
                    };
                    break;
                case SCOT_DisConnAll:
                    {	//删除所有客户端
                        pServer->OnDisConnAll();
                    }
                    break;
                case SCOT_CloseAll:
                    {
                        pServer->OnCloseAll();
                    }
                    break;
                case SCOT_Send:
                    {
                        CServerClient* pClient = pServer->FindClient(pSocketOper->lSocketID);
                        if(pClient)
                        {
                            pClient->AddSendMsg((CBaseMessage*)pSocketOper->pBuf);
                        }
                        else
                        {
                            CBaseMessage* pMsg = (CBaseMessage*)pSocketOper->pBuf;
                            if(pMsg->RemoveRefCount() == 0)
                                CBaseMessage::FreeMsg(pMsg);
                        }
                    }
                    break;
                case SCOT_OnSend:
                    {
                        pServer->OnSend((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    }
                    break;
                case SCOT_OnSendZeroByte:
                    {
                        pServer->OnSendZeorByte((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID);
                    }
                    break;
                case SCOT_OnRecieve:
                    {
                        pServer->OnReceive((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    }
                    break;
                case SCOT_OnClose:
                    {
                        pServer->OnClose((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID);
                    };
                    break;
                case SCOT_OnError:
                    {
                        pServer->OnError((PER_IO_OPERATION_DATA*)pSocketOper->pBuf,
                            pSocketOper->lSocketID,
                            pSocketOper->lNum1);
                    };
                    break;
                case SCOT_ExitThread:
                    {
                        bRun = false;
                    }
                    break;
                }
                pServer->FreeSockOper(pSocketOper);
            }
        }

        TemptCommands.clear();
        //Sleep(1);
    }
}

//网络主线程，对网络的操作命令进行解析，执行
unsigned __stdcall NetThreadFunc(void* pArguments)
{
    CServer* pServer=(CServer*)pArguments;
    if(pServer==NULL)
    {
        _endthreadex( 0 );
        return 0;
    }

#ifndef _DEBUG
    __try
    {
#endif

        DoNetThreadFunc(pServer);

#ifndef _DEBUG
    }
    __except(_Sword3::CrashFilter(GetExceptionInformation(),GetExceptionCode()))
    {
        PutErrorString(NET_MODULE,"%-15s %s",__FUNCTION__,"********服务器工作线程出错，请查看最新的错误报告文件********");
    }
#endif

    _endthreadex( 0 );
    return 0;
}
