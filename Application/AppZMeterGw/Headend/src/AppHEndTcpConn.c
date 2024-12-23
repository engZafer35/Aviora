/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 19, 2024 - 8:56:55 PM
* #File Name    : AppHEndTcpConn.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppHEndTcpConn.h"

#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"
#include "AppMessageHandlerManager.h"

#include "net_config.h"
/****************************** MACRO DEFINITIONS *****************************/
#define MAX_SERVER_NUM (2)
/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
struct client
{
    char     cliName[16];
    int      cliSock;
    OsQueue  cliSndQue;
//    Msg_Handler_Message msg;
};

static struct stcServer
{
    char name[16];
    struct client cli[3];
    int masterSocket;
    int maxClient;
    int activeClientNum;

    void *msgHandler;
    OsTaskId task;
    OsTaskId taskSender;
    MESSAGE_TYPE_T msgType;
}gs_tcpServer[MAX_SERVER_NUM];
/***************************** STATIC FUNCTIONS  ******************************/
static void serverSenderTask(void *pvParameters)
{
    struct stcServer *server = (struct stcServer *)pvParameters;
    U32 z;
    S32 rLen; //received data length
    char buff[1024];

    while(1)
    {
        for (z = 0; z < server->maxClient; z++)
        {
            if ((OS_INVALID_QUEUE != server->cli[z].cliSndQue) && (0 < (rLen = zosMsgQueueReceive(server->cli[z].cliSndQue, buff, 2048, WAIT_10_MS))))
            {
                SEND(server->cli[z].cliSock, buff, rLen , 0 );
                printf("Viko Reply: RLen: %d - %s \n\r", rLen, buff);
            }
        }

        sleep(1);
    }

}

static void serverTask(void * pvParameters)
{
    struct stcServer *server = (struct stcServer *)pvParameters;
    int newSocket, activity, i ,sd;
    int max_sd;
    struct sockaddr_in address;
    int rcvSize;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    zosDelayTask(1000);

    while(TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(server->masterSocket, &readfds);
        max_sd = server->masterSocket;

        //add child sockets to set
        for (i = 0 ; i < server->maxClient ; i++)
        {
            //socket descriptor
            sd = server->cli[i].cliSock;

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET(sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = SELECT( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0))
        {
            DEBUG_ERROR("SELECT ERROR %s", server->name);
        }

        DEBUG_DEBUG("->[I] Server Name: %s", server->name); //This log will describe the following logs, user can understand which server it belongs to
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(server->masterSocket, &readfds))
        {
            int addrlen = sizeof(struct sockaddr_in);
            if ((newSocket = ACCEPT(server->masterSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                DEBUG_ERROR("->[E] ACCEPT Error %s", server->name);
                continue;
            }

            DEBUG_DEBUG("->[I] New connection , socket fd is %d , ip is : %s , port : %d" , newSocket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            if (server->activeClientNum == server->maxClient)
            {
                DEBUG_DEBUG("->[D] New client connection is rejected. Client List full %d", server->activeClientNum);
                CLOSESOCKET(newSocket);
                continue;
            }

            server->activeClientNum++;

            //add new socket to array of sockets
            for (i = 0; i < server->maxClient; i++)
            {
                //if position is empty
                if (server->cli[i].cliSock == 0 )
                {
                    DEBUG_INFO("->[I] Adding to list of sockets as %d\n" , i);
                    server->cli[i].cliSock = newSocket;
                    sprintf(server->cli[i].cliName, "CLI%d", newSocket);  //client socket number is used also client name
                    server->cli[i].cliSndQue = appMsgHandlerAddClient(server->cli[i].cliName);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < server->maxClient; i++)
        {
            sd = server->cli[i].cliSock;

            if (FD_ISSET(sd , &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((rcvSize = RECV(sd , buffer, 1024, 0)) == 0)
                {
                    //Somebody disconnected
                    int addrlen = sizeof(struct sockaddr_in);
                    GETPEERNAME(sd, (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    DEBUG_WARNING("->[W] Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //delete client from mesage handler list
                    appMsgHandlerRemoveClient(server->cli[i].cliName);
                    server->cli[i].cliSndQue = OS_INVALID_QUEUE;

                    //Close the socket and mark as 0 in list for reuse
                    CLOSESOCKET(sd);
                    server->cli[i].cliSock = 0;
                    server->activeClientNum--;
                    memset(server->cli[i].cliName, 0, sizeof(server->cli[i].cliName));
                }
                //Echo back the message that came in
                else
                {
                    Msg_Handler_Message msg; //handleMsg will copy buffer, so msg could be local variable.
                    msg.msgType = server->msgType;
                    msg.data    = buffer;
                    msg.length  = rcvSize;
                    appMsgHandlerHandleMsg(server->cli[i].cliName, &msg);
                }
            }
        }
    }
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appHEndTcpOpenServer(const char *hEndName, const char *ip, U32 port, U32 maxClient, void *packetHandlerFunc, MESSAGE_TYPE_T msgType)
{
    RETURN_STATUS retVal = FAILURE;
    struct stcServer *server = NULL;
    U32 i;
    int opt = TRUE;
    struct sockaddr_in address;


    if (IS_NULL_PTR(hEndName) || IS_NULL_PTR(ip))
    {
        return retVal;
    }

    for (i = 0; i < MAX_SERVER_NUM; i++)
    {
        if ('\0' == gs_tcpServer[i].name[0]) //if first char of name is '\0', this server is empty
        {
            server = &gs_tcpServer[i];
            break;
        }
    }

    if (NULL != server)
    {
        //create a master socket
        if ((server->masterSocket = SOCKET(AF_INET , SOCK_STREAM , 0)) == -1)
        {
            DEBUG_ERROR("Master Socket Failed, name: %s", hEndName);
            return FAILURE;
        }

        /* set master socket to allow multiple connections , this is just a good habit, it will work without this
         * So dont need to handle return value.
         */
        SETSOCKOPT(server->masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

        //type of socket created
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port        = htons(port);

        if (BIND(server->masterSocket, (struct sockaddr *)&address, sizeof(address))<0)
        {
            DEBUG_ERROR("SOCKET BIND Failed, name: %s", hEndName);
            return FAILURE;
        }

        //try to specify maximum of 3 pending connections for the master socket
        if (LISTEN(server->masterSocket, 3) < 0)
        {
            DEBUG_ERROR("SOCKET LISTEN Failed, name: %s", hEndName);
            return FAILURE;
        }

        ZOsTaskParameters tempParam;
        tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
        tempParam.stackSize = ZOS_MIN_STACK_SIZE;

        //every thing is ok so far, set values.
        strcpy(server->name, hEndName);
        server->maxClient  = maxClient;
        server->msgHandler = packetHandlerFunc;
        server->msgType    = msgType;

        //the following lines could be invoked in for loop
        server->cli[0].cliSndQue = OS_INVALID_QUEUE;
        server->cli[1].cliSndQue = OS_INVALID_QUEUE;
        server->cli[2].cliSndQue = OS_INVALID_QUEUE;

        retVal = SUCCESS;

        server->task = appTskMngCreate(hEndName, serverTask, server, &tempParam, FALSE);
        if (OS_INVALID_TASK_ID == server->task)
        {
            CLOSESOCKET(server->masterSocket);
            retVal = FAILURE;
        }
        else
        {
            char tname[16] = "";
            sprintf(tname, "%s%s", hEndName, "S"); //create a name for sender task, added just S end of the string
            server->taskSender = appTskMngCreate(tname, serverSenderTask, server, &tempParam, FALSE);
            if (OS_INVALID_TASK_ID == server->taskSender)
            {
                appTskMngDelete(server->task);
                CLOSESOCKET(server->masterSocket);
                retVal = FAILURE;
            }
        }

        if (FAILURE == retVal)
        {
            memset(server->name, '\0', sizeof(server->name));
            server->maxClient = 0;
            server->msgHandler = NULL;
        }
    }

    return retVal;
}

RETURN_STATUS appHEndTcpCloseServer(const char *hEndName)
{
    return SUCCESS;
}

RETURN_STATUS appHEndTcpConnect(const char *hEndName, const char *ip, U32 port, void *packetHandlerFunc)
{
    return SUCCESS;
}

RETURN_STATUS appHEndTcpDisconnect(const char *hEndName)
{
    return SUCCESS;
}
/******************************** End Of File *********************************/
