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
static struct stcServer
{
    char name[16];
    int masterSocket;
    int clientSock[3];
    int maxClient;
    int activeClientNum;

    void *msgHandler;
    OsTaskId task;
    MESSAGE_TYPE_T msgType;
}gs_tcpServer[MAX_SERVER_NUM];
/***************************** STATIC FUNCTIONS  ******************************/

static void serverTask(void * pvParameters)
{
    struct stcServer *server = (struct stcServer *)pvParameters;
    int new_socket, activity, i ,sd;
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
            sd = server->clientSock[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

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

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(server->masterSocket, &readfds))
        {
            int addrlen = sizeof(struct sockaddr_in);
            if ((new_socket = ACCEPT(server->masterSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                DEBUG_ERROR("->[E] ACCEPT Error %s", server->name);
                continue;
            }

            server->activeClientNum++;
            DEBUG_DEBUG("->[I] New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //send new connection greeting message
            //SEND(new_socket, "DDOG-ZMG", strlen("DDOG-ZMG"), 0);

            //add new socket to array of sockets
            for (i = 0; i < server->maxClient; i++)
            {
                //if position is empty
                if (server->clientSock[i] == 0 )
                {
                    server->clientSock[i] = new_socket;
                    DEBUG_INFO("->[I]Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < server->maxClient; i++)
        {
            sd = server->clientSock[i];

            if (FD_ISSET(sd , &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((rcvSize = RECV(sd , buffer, 1024, 0)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    int addrlen = sizeof(struct sockaddr_in);
                    GETPEERNAME(sd, (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    DEBUG_WARNING("->[W] Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    CLOSESOCKET(sd);
                    server->clientSock[i] = 0;
                    server->activeClientNum--;
                }
                //Echo back the message that came in
                else
                {
                    Msg_Handler_Message msg;

                    msg.msgType = server->msgType;
                    msg.data = buffer;
                    msg.length = rcvSize;
                    appMsgHandlerHandleMsg(server->name, &msg);
                    //set the string terminating NULL byte on the end of the data read
                    char buff[1024];
                    int rlen;
                    extern OsQueue testQueue ;
                    extern OsQueue testQueue2 ;

                    OsQueue *queue = NULL;

                    if (EN_MSG_TYPE_VIKO == server->msgType)         queue = &testQueue;
                    else if (EN_MSG_TYPE_GRIDBOX == server->msgType) queue = &testQueue2;

                    sleep(2);
                    while(1)
                    {
                        if ((*queue != OS_INVALID_QUEUE) && (0 < (rlen = zosMsgQueueReceive(*queue, buff, 2048, WAIT_10_MS))))
                        {
                            SEND(sd, buff, rlen , 0 );
                            printf("Viko Reply: RLen: %d - %s \n\r", rlen, buff);
                            break;
                        }
                    }

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
        server->maxClient = maxClient;
        server->msgHandler = packetHandlerFunc;
        server->msgType = msgType;

        retVal = SUCCESS;

        server->task = appTskMngCreate(hEndName, serverTask, server, &tempParam, FALSE);
        if (OS_INVALID_TASK_ID == server->task)
        {
            CLOSESOCKET(server->masterSocket);
            retVal = FAILURE;
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

#if 0
/**
    Handle multiple socket connections with select and fd_set on Linux
    Raj Abishek <rajabishek@hotmail.com>
*/

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRUE   1
#define FALSE  0
#define PORT 8888

int main(int argc , char *argv[])
{
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char *message = "ECHO Daemon v1.0 \r\n";

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while(TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }

                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    send(sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }

    return 0;
}
#endif
