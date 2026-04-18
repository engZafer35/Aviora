/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 2.0
* #Date         : Mar 28, 2026 - 18:05:26 PM
* #File Name    : AppTcpConnManager.c
*******************************************************************************/
/******************************************************************************
* This file is used to manage the TCP connection for the gateway.

*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppTcpConnManager.h"
#include "AppTaskManager.h"
#include "AppLogRecorder.h"

#include "net_config.h"
/****************************** MACRO DEFINITIONS *****************************/
#define RET_SUCCESS    (0)
#define RET_FAILURE    (-1)

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
static union TaskList
{
#define MAX_INTERNAL_MAX_TASK_NUMBER    (32)

    unsigned int taskAll;
    struct
    {
        unsigned int taskConnectPush     : 1;
        unsigned int taskSendPushPacket  : 1;
        unsigned int taskClosePushSock   : 1;

        unsigned int taskPullSocCreat    : 1;
        unsigned int taskClosePullSock   : 1;

        unsigned int taskKillThread      : 1;

        unsigned int taskKeepDisconnect  : 1;

        unsigned int infoPushConnected   : 1;
    };
}gs_taskList;

static struct
{
    int pullSockID;
    int pushSockID;
    int pushSockConnecting;

    int clientSocList[MAX_PULL_CLIENT_NUMBER];
    int clietKeepAliveCounter[MAX_PULL_CLIENT_NUMBER];
}gs_connInfo;

static const char *gs_serverIP;
static int gs_serverPort;
static int gs_pullPort;
static IncomingMsngCb_t gs_incomingMsngCb;

static OsTaskId gs_tcpTaskId;

/***************************** STATIC FUNCTIONS  ******************************/
static int isTherePushTxFile(void)
{
    return FALSE;
}

static int closePullSocket(void)
{
    int retVal = RET_SUCCESS;
    int i;

    for (i = 0; i < MAX_PULL_CLIENT_NUMBER; i++)
    {
        if (gs_connInfo.clientSocList[i] > 0)
        {
            CLOSESOCKET(gs_connInfo.clientSocList[i]);

            gs_connInfo.clientSocList[i] = 0;
            gs_connInfo.clietKeepAliveCounter[i] = 0;

            DEBUG_INFO("->[I] Pull Socket, Client %d socket is closed", i);
        }
    }

    if (gs_connInfo.pullSockID >= 0)
    {
        retVal = RET_FAILURE;

        CLOSESOCKET(gs_connInfo.pullSockID);
        gs_connInfo.pullSockID = -1;
        retVal = RET_SUCCESS;
        DEBUG_INFO("->[I] Pull Socket, Closed !!");
    }

    return retVal;
}

static int createPullSocket(void)
{
    int retVal = RET_FAILURE;
    struct sockaddr_in local_ipv4_address = { 0 };
    int sock = SOCKET(AF_INET, SOCK_STREAM, 0);

    if (-1 == sock) return retVal;

    int reuse = TRUE;
    SETSOCKOPT(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    local_ipv4_address.sin_family      = AF_INET;
    local_ipv4_address.sin_addr.s_addr = INADDR_ANY;
    local_ipv4_address.sin_port        = htons(gs_pullPort);

    if (BIND(sock, (const struct sockaddr *)&local_ipv4_address, sizeof(local_ipv4_address)) >= 0)
    {
        if (LISTEN(sock, 1) >= 0)
        {
            gs_connInfo.pullSockID = sock;
            retVal = RET_SUCCESS;
            DEBUG_INFO("->[I] Pull Socket is created, Port: %d", gs_pullPort);
        }
        else
        {
            gs_connInfo.pullSockID = -1;
        }
    }

    if (RET_FAILURE == retVal)
    {
        CLOSESOCKET(sock);
        zosDelayTask(500);
        DEBUG_ERROR("->[E] Pull Socket could not be created");
        APP_LOG_REC(g_sysLoggerID, "Pull Socket could not be created");
    }

    return retVal;
}

static int disconnectPushSocket(void)
{
    int retVal = RET_SUCCESS;

    if (gs_connInfo.pushSockID >= 0)
    {
        CLOSESOCKET(gs_connInfo.pushSockID);
        gs_connInfo.pushSockID = -1;
        DEBUG_DEBUG("->[D] Push Socket Closed");
    }

    return retVal;
}

static int connectPushSocket(void)
{
    int retVal = RET_SUCCESS;
    struct sockaddr_in server_ipv4_address = { 0 };
    int flags;

    if (gs_connInfo.pushSockID >= 0)
    {
        return RET_SUCCESS;
    }

    gs_connInfo.pushSockID = SOCKET(AF_INET, SOCK_STREAM, 0);

    if (gs_connInfo.pushSockID < 0)
    {
        return RET_FAILURE;
    }

    flags = FCNTL_COM(gs_connInfo.pushSockID, F_GETFL, flags);
    if ((flags < 0) || (FCNTL_COM(gs_connInfo.pushSockID, F_SETFL, flags | O_NONBLOCK) < 0))
    {
        CLOSESOCKET(gs_connInfo.pushSockID);
        gs_connInfo.pushSockID = -1;
        return RET_FAILURE;
    }

    server_ipv4_address.sin_family = AF_INET;
    server_ipv4_address.sin_port = htons(gs_serverPort);

    if (inet_aton(gs_serverIP, &server_ipv4_address.sin_addr.s_addr) == 0)
    {
        retVal = RET_FAILURE;
    }

    if (RET_SUCCESS == retVal)
    {
        int connect_result = CONNECT(gs_connInfo.pushSockID, (const struct sockaddr *)&server_ipv4_address, sizeof(server_ipv4_address));
        if (connect_result < 0)
        {
            DEBUG_DEBUG("->[D] Push socket connection initiated (non-blocking)...");
            gs_connInfo.pushSockConnecting = 1;
            retVal = RET_SUCCESS;
        }
        else if (connect_result == 0)
        {
            gs_connInfo.pushSockConnecting = 0;
            retVal = RET_SUCCESS;
        }
    }

    if (RET_FAILURE == retVal)
    {
        CLOSESOCKET(gs_connInfo.pushSockID);
        gs_connInfo.pushSockID = -1;
        DEBUG_ERROR("->[E] Push socket could not be connected !!");
        APP_LOG_REC(g_sysLoggerID, "Push socket could not be connected !!");
    }

    return retVal;
}

static void tcpConnectionThread(void *arg)
{
    (void)arg;
    FD_SET_TYPE readfds;
    FD_SET_TYPE writefds;
    int max_sd;
    int activity;
    static char ls_tmpRcvbuff[1024];
    int rcvSize;
    int i;

    int pushSockCounter = 0;

    STRUCT_TIMEVAL timeout;

    while (1)
    {
        if ((TRUE == gs_taskList.taskKeepDisconnect) && ((gs_connInfo.pullSockID >= 0) || (gs_connInfo.pushSockID >= 0)))
        {
            gs_taskList.taskClosePushSock = TRUE;
            gs_taskList.taskClosePullSock = TRUE;
        }

        /*****************************************************/
        if (TRUE == gs_taskList.taskPullSocCreat)
        {
            if (gs_connInfo.pullSockID >= 0 || RET_SUCCESS == createPullSocket())
            {
                gs_taskList.taskPullSocCreat = FALSE;
            }
        }

        if (TRUE == gs_taskList.taskClosePullSock)
        {
            if (RET_SUCCESS == closePullSocket())
            {
                gs_taskList.taskClosePullSock = FALSE;
            }
        }

        /*****************************************************/
        if (TRUE == gs_taskList.taskConnectPush)
        {
            if (RET_SUCCESS == connectPushSocket())
            {
                DEBUG_DEBUG("->[D] Push Socket, Connection initiated");
                gs_taskList.taskConnectPush = FALSE;
            }
        }

        if (TRUE == gs_taskList.taskClosePushSock)
        {
            if (RET_SUCCESS == disconnectPushSocket())
            {
                gs_taskList.taskClosePushSock = FALSE;
            }
        }

        /******************************* Pull Socket *****************************/
        if (gs_connInfo.pullSockID >= 0)
        {
//            FD_SET_TYPE fdSet;
//            S32 g_TcpClientFd = -1;
//            struct sockaddr_in clientAddr;
//            SOCKLEN_t clientLen = sizeof(clientAddr);
//            U8 cliVoice[160];
//                while(1)
//                {
//                    FD_ZERO(&fdSet);
//                    FD_SET(gs_connInfo.pullSockID, &fdSet);
//
//                    timeout.tv_sec  = 1;
//                    timeout.tv_usec = 10000;
//
//                    // TCP server wait for connection
//                    SELECT(FD_SETSIZE, &fdSet, NULL, NULL, &timeout);
//
//                    if(FD_ISSET(gs_connInfo.pullSockID, &fdSet))
//                    {
//                        g_TcpClientFd = ACCEPT(gs_connInfo.pullSockID,
//                                               (struct sockaddr*)&clientAddr,
//                                               &clientLen);
//
//                        if(g_TcpClientFd >= 0)
//                        {
//
//
//                            int recvLen = RECV(g_TcpClientFd,
//                                                cliVoice,
//                                                sizeof(cliVoice),
//                                                0);
//
//                            if(recvLen > 0)
//                            {
//                                for (int i = 0; i < recvLen ; i++)
//                                 printf("%c", cliVoice[i]);
//                                printf("\r\n");
//
//                            }
//
//                            CLOSESOCKET(g_TcpClientFd);
//                            g_TcpClientFd = -1;
//                        }
//                    }
//                }






            FD_ZERO(&readfds);

            FD_SET(gs_connInfo.pullSockID, &readfds);
            max_sd = gs_connInfo.pullSockID;

            for (i = 0 ; i < MAX_PULL_CLIENT_NUMBER ; i++)
            {
                if (gs_connInfo.clientSocList[i] > 0)
                    FD_SET(gs_connInfo.clientSocList[i], &readfds);

                if (gs_connInfo.clientSocList[i] > max_sd)
                    max_sd = gs_connInfo.clientSocList[i];
            }

            timeout.tv_sec  = 0;
            timeout.tv_usec = 200000;
            activity = SELECT(max_sd + 1, &readfds, NULL, NULL, &timeout);

            if (activity > 0)
            {
                if (FD_ISSET(gs_connInfo.pullSockID, &readfds))
                {
                    struct sockaddr_in client_ipv4_address = { 0 };
                    int addrlen = sizeof(client_ipv4_address);

                    int newSocket = ACCEPT(gs_connInfo.pullSockID, (struct sockaddr *)&client_ipv4_address, (SOCKLEN_t *)&addrlen);
                    if (newSocket <= 0)
                    {
                        DEBUG_ERROR("->[E] Pull Socket, ACCEPT Error.");
                        continue;
                    }

                    DEBUG_DEBUG("->[I] New connection , socket fd is %d , ip is : %s , port : %d", newSocket, inet_ntoa(client_ipv4_address.sin_addr), ntohs(client_ipv4_address.sin_port));

                    for (i = 0; i < MAX_PULL_CLIENT_NUMBER; i++)
                    {
                        if (gs_connInfo.clientSocList[i] <= 0)
                        {
                            DEBUG_INFO("->[I] Pull Socket, New client is added. Index is %d", i);
                            gs_connInfo.clientSocList[i] = newSocket;
                            gs_connInfo.clietKeepAliveCounter[i] = 0;
                            break;
                        }
                    }

                    if (MAX_PULL_CLIENT_NUMBER == i)
                    {
                        DEBUG_DEBUG("->[D] Pull Socket, New client connection is rejected. Client List full");
                        CLOSESOCKET(newSocket);
                        continue;
                    }
                }

                for (i = 0; i < MAX_PULL_CLIENT_NUMBER; i++)
                {
                    if ((gs_connInfo.clientSocList[i] > 0) && FD_ISSET(gs_connInfo.clientSocList[i], &readfds))
                    {
                        rcvSize = RECV(gs_connInfo.clientSocList[i], ls_tmpRcvbuff, sizeof(ls_tmpRcvbuff), 0);
                        if (rcvSize <= 0)
                        {
                            DEBUG_DEBUG("->[D] Pull Socket, Client disconnected. Index %d", i);
                            CLOSESOCKET(gs_connInfo.clientSocList[i]);
                            gs_connInfo.clientSocList[i] = 0;
                            gs_connInfo.clietKeepAliveCounter[i] = 0;
                        }
                        else
                        {
                            gs_connInfo.clietKeepAliveCounter[i] = 0;
                            DEBUG_DEBUG("->[D] Pull Socket, Data received from Client- %d. Data size: %d", i, rcvSize);

                            gs_incomingMsngCb(PULL_TCP_SOCK_NAME, ls_tmpRcvbuff, rcvSize);
                        }
                    }
                }
            }
            else if (0 == activity)
            {
                for (i = 0; i < MAX_PULL_CLIENT_NUMBER; i++)
                {
                    gs_connInfo.clietKeepAliveCounter[i] += gs_connInfo.clientSocList[i] > 0 ? 1 : 0;

                    if (gs_connInfo.clietKeepAliveCounter[i] > 36000) //360 sec = ~6 min (with 10ms timeout)
                    {
                        CLOSESOCKET(gs_connInfo.clientSocList[i]);
                        gs_connInfo.clientSocList[i] = 0;
                        gs_connInfo.clietKeepAliveCounter[i] = 0;

                        DEBUG_DEBUG("->[D] Pull Socket, client %d not Alive, closed the socket", i);
                    }
                }
            }
        }
        /**************************** End of Pull Socket *************************/


        /******************************* Push Socket *****************************/
        if (gs_connInfo.pushSockID >= 0)
        {
            FD_ZERO(&readfds);
            FD_SET(gs_connInfo.pushSockID, &readfds);

            max_sd = gs_connInfo.pushSockID;
            timeout.tv_sec  = 0;
            timeout.tv_usec = 10000;

            if (gs_connInfo.pushSockConnecting)
            {
                FD_ZERO(&writefds);
                FD_SET(gs_connInfo.pushSockID, &writefds);
                activity = SELECT(max_sd + 1, &readfds, &writefds, NULL, &timeout);
            }
            else
            {
                activity = SELECT(max_sd + 1, &readfds, NULL, NULL, &timeout);
            }

            if (activity > 0)
            {
                if (gs_connInfo.pushSockConnecting && FD_ISSET(gs_connInfo.pushSockID, &writefds))
                {
                    int error = 0;
                    SOCKLEN_t len = sizeof(error);

                    if (GETSOCKOPT(gs_connInfo.pushSockID, SOL_SOCKET, SO_ERROR, &error, &len) == 0)
                    {
                        gs_connInfo.pushSockConnecting = 0;
                        if (error == 0 || (EWOULDBLOCK == error))
                        {
                            DEBUG_DEBUG("->[D] Push socket connected successfully");
                            int fl = FCNTL(gs_connInfo.pushSockID, F_GETFL, 0);
                            FCNTL(gs_connInfo.pushSockID, F_SETFL, fl & ~O_NONBLOCK);
                        }
                        else
                        {
                            DEBUG_ERROR("->[E] Push socket connection failed: %d", error);
                            CLOSESOCKET(gs_connInfo.pushSockID);
                            gs_connInfo.pushSockID = -1;
                            pushSockCounter = 0;
                        }
                    }
                }
                if (FD_ISSET(gs_connInfo.pushSockID, &readfds))
                {
                    rcvSize = RECV(gs_connInfo.pushSockID, ls_tmpRcvbuff, sizeof(ls_tmpRcvbuff), 0);
                    if (rcvSize <= 0)
                    {
                        DEBUG_DEBUG("->[D] Push Socket, Push Socket is closed by Headend");

                        CLOSESOCKET(gs_connInfo.pushSockID);
                        gs_connInfo.pushSockID = -1;

                        pushSockCounter = 0;
                    }
                    else
                    {
                        DEBUG_DEBUG("->[D] Push Socket, Data received. Data size: %d", rcvSize);

                        gs_incomingMsngCb(PUSH_TCP_SOCK_NAME, ls_tmpRcvbuff, (unsigned int)rcvSize);
                    }
                }
            }
            else if (activity == 0)
            {
                pushSockCounter++;
            }
        }
        else if ((FALSE == gs_taskList.taskKeepDisconnect) && (isTherePushTxFile() > 0))
        {
            gs_taskList.taskConnectPush = TRUE;
            DEBUG_DEBUG("->[D] Push Socket, Push will be connected for pending TX file");
        }
    }
}
/***************************** PUBLIC FUNCTIONS  ******************************/
int appTcpConnManagerStart(const char *serverIP, int serverPort, int pullPort, IncomingMsngCb_t incomingMsngCb)
{
    int retVal = RET_SUCCESS;

    gs_taskList.taskKeepDisconnect = TRUE;

    gs_connInfo.pullSockID = -1;
    gs_connInfo.pushSockID = -1;

    gs_serverIP   = serverIP;
    gs_serverPort = serverPort;
    gs_pullPort   = pullPort;
    gs_incomingMsngCb = incomingMsngCb;

    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = ZOS_MIN_STACK_SIZE*5;

    gs_tcpTaskId = appTskMngCreate("TcpConn", tcpConnectionThread, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_tcpTaskId)
    {
        DEBUG_ERROR("TCP Connection task not created !");
        APP_LOG_REC(g_sysLoggerID, "TCP Connection task not created !");
        retVal = RET_FAILURE;

        gs_serverIP   = NULL;
        gs_serverPort = 0;
        gs_pullPort   = 0;
        gs_incomingMsngCb = NULL;
    }
    else
    {
        DEBUG_INFO("TCP Connection task created !");
        APP_LOG_REC(g_sysLoggerID, "TCP Connection task created !");
    }

    return retVal;
}

int appTcpConnManagerStop(void)
{
    closePullSocket();
    zosDelayTask(1000);

    disconnectPushSocket();

    gs_taskList.taskAll = 0;

    if (OS_INVALID_TASK_ID != gs_tcpTaskId)
    {
        DEBUG_INFO("TCP Connection Stopped");
        APP_LOG_REC(g_sysLoggerID, "TCP Connection Stopped");

        appTskMngDelete(&gs_tcpTaskId);
        gs_tcpTaskId = OS_INVALID_TASK_ID;
    }

    return RET_SUCCESS;
}

BOOL appTcpConnManagerAnyPullClient(void)
{
    if (gs_connInfo.pullSockID < 0)
    {
        return FALSE;
    }
    for (int i = 0; i < MAX_PULL_CLIENT_NUMBER; i++)
    {
        if (gs_connInfo.clientSocList[i] > 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL appTcpConnManagerIsPullReady(void)
{
    return (gs_connInfo.pullSockID >= 0);
}

BOOL appTcpConnManagerIsConnectedPush(void)
{
    return ((gs_connInfo.pushSockID > 0) && (gs_connInfo.pushSockConnecting == 0));
}

void appTcpConnManagerRequestPushConnect(void)
{
    gs_taskList.taskConnectPush = TRUE;
}

void appTcpConnManagerRequestPushDisconnect(void)
{
    gs_taskList.taskClosePushSock = TRUE;
}

int appTcpConnManagerRequestConnect(void)
{
    gs_taskList.taskKeepDisconnect = FALSE;
    gs_taskList.taskPullSocCreat   = TRUE;

    return RET_SUCCESS;
}

int appTcpConnManagerRequestDisconnect(void)
{
    gs_taskList.taskKeepDisconnect = TRUE;
    DEBUG_DEBUG("User requested to disconnect, push and pull sock will be closed");

    return RET_SUCCESS;
}

int appTcpConnManagerSend(const char *ch, const char *data, unsigned int dataLeng)
{
    int retVal = RET_FAILURE;

    if (0 == strcmp(PUSH_TCP_SOCK_NAME, ch))
    {
        if (FALSE == appTcpConnManagerIsConnectedPush())
        {
            gs_taskList.taskConnectPush = TRUE;
            DEBUG_DEBUG("Push sock will be open and data will be sent");
        }
        else
        {
            retVal = RET_SUCCESS;
            if (SEND(gs_connInfo.pushSockID, data, dataLeng, 0) < 0)
            {
                DEBUG_ERROR("Push sock data not sent !");
                APP_LOG_REC(g_sysLoggerID, "Push sock data not sent !");
                retVal = RET_FAILURE;
            }
        }
    }
    else if (0 == strcmp(PULL_TCP_SOCK_NAME, ch))
    {
        if (gs_connInfo.pullSockID > 0)
        {
            retVal = RET_SUCCESS;
            if (SEND(gs_connInfo.clientSocList[0], data, dataLeng, 0) < 0)
            {
                DEBUG_ERROR("Pull sock data not sent !");
                APP_LOG_REC(g_sysLoggerID, "Pull sock data not sent !");
                
                retVal = RET_FAILURE;
            }
        }
    }

    return retVal;
}
/******************************** End Of File *********************************/
