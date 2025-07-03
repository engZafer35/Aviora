/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jul 10, 2024 - 3:21:52 PM
* #File Name    : AppVikoProtocol.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppVikoProtocol_.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"
#include "AppInternalMsgFrame.h"
#include "AppTaskManager.h"

#include "net_config.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
static S32 gs_vikoDbusID;
static S32 gs_devMsgSN;
static OsTaskId gs_vikoTaskID;
/***************************** STATIC FUNCTIONS  ******************************/

static RETURN_STATUS openPushSocket(void)
{
    int opt = TRUE;
    int masterSocket , addrlen , new_socket , clientSocket[MAX_CLIENT_NUM], activity, valread , sd;

    int max_sd;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[1025];

    //initialise all clientSocket[] to 0 so not checked
    for (U32 i = 0; i < MAX_CLIENT_NUM; i++)
    {
        clientSocket[i] = 0;
    }

    //create a master socket
    if( (masterSocket = SOCKET(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        //perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( SETSOCKOPT(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        //perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_LOOPBACK; // INET_ADDR(PUSH_SOCKET_IP); //TODO: use device IP
    address.sin_port = htons(PUSH_SOCKET_PORT);

    //bind the socket to localhost port 8888
    if (BIND(masterSocket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        //perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //printf("Listener on port %d \n", PUSH_SOCKET_PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (LISTEN(masterSocket, 3) < 0)
    {
        //perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    printf("Waiting for connections ...");

    while(TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(masterSocket, &readfds);
        max_sd = masterSocket;

        //add child sockets to set
        for (U32 i = 0 ; i < MAX_CLIENT_NUM ; i++)
        {
            //socket descriptor
            sd = clientSocket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = SELECT( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if (activity < 0)
        {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(masterSocket, &readfds))
        {
            if ((new_socket = ACCEPT(masterSocket, (struct sockaddr *)&address, (SOCKLEN_t*)&addrlen))<0)
            {
                //perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //send new connection greeting message
            if( SEND(new_socket, "ZAFER\n\r", 8, 0) != 8 )
            {
                printf("sending error !!");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (U32 i = 0; i < MAX_CLIENT_NUM; i++)
            {
                //if position is empty
                if( clientSocket[i] == 0 )
                {
                    clientSocket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (U32 i = 0; i < MAX_CLIENT_NUM; i++)
        {
            sd = clientSocket[i];

            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = RECV(sd, buffer, 1024, 0)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (SOCKLEN_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    clientSocket[i] = 0;
                }

                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    SEND(sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }
}

static void displayTask(void * pvParameters)
{
    zosDelayTask(1000); //wait once before starting


    while (1)
    {

        appTskMngImOK(gs_vikoTaskID);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appVikoInit(const char *servIP, int port)
{
    RETURN_STATUS retVal = SUCCESS;
    retVal = appDBusRegister(EN_DBUS_TOPIC_NETWORK | EN_DBUS_TOPIC_DEVICE, &gs_vikoDbusID);

    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    gs_vikoTaskID = appTskMngCreate("DISPLAY_TASK", displayTask, NULL, &tempParam);

    return retVal;
}

void appVikoTaskStart(void)
{

}

void appVikoTaskStop(void)
{

}

/******************************** End Of File *********************************/
