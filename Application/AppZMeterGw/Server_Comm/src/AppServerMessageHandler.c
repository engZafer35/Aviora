/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Nov 18, 2024 - 11:42:13 AM
* #File Name    : AppServerMessageHandler.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppServerMessageHandler.h"
#include "AppTaskManager.h"

/****************************** MACRO DEFINITIONS *****************************/
#define MAX_MSG_BUFFER_NUM          (3)

#define MAX_CLIENT_CIRCULAR_SIZE    (5)
#define MAX_CLIENT_REPLY_BUFF_SIZE  (512)  //byte

#define MAX_CLIENT_NAME_LENG        (16)  //byte

#define MAX_MSG_HANDLER_LIST_NUM    (5)
/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
struct clientReplyCircularBuffer
{
    char buffer[MAX_CLIENT_CIRCULAR_SIZE][MAX_CLIENT_REPLY_BUFF_SIZE];
    int head;
    int tail;
    int currIndex;
};

typedef struct Client
{
    Msg_Handler_Message message[MAX_MSG_BUFFER_NUM];
    U32 rcvMsgHead;
    U32 rcvMsgTail;
    char name[MAX_CLIENT_NAME_LENG];
    OsQueue que;
    struct clientReplyCircularBuffer reply;
    struct Client *next;
}Client_t;

static Client_t *gs_Clients = NULL;

static struct
{
    char name[16];
    MsgHandler_t msgHandler;
}gs_msgHandlerList[MAX_MSG_HANDLER_LIST_NUM];

//tatic MsgHandler_t gs_msgHandlerList[MAX_MSG_HANDLER_LIST_NUM];

static OsQueue gs_rcvMsgQue;
static OsTaskId gs_msgHandTaskID;
/***************************** STATIC FUNCTIONS  ******************************/
static RETURN_STATUS sendReplyMessage(Client_t *client, const U8 *msg, U32 msgLeng)
{
    RETURN_STATUS retVal = FAILURE;

    memcpy(client->reply.buffer[client->reply.currIndex], msg, msgLeng);

    if (QUEUE_SUCCESS == zosMsgQueueSend(client->que, (const char *)client->reply.buffer[client->reply.currIndex], msgLeng, TIME_OUT_10MS))
    {
        retVal = SUCCESS;
    }

    client->reply.currIndex++; //set next position

    if (client->reply.currIndex >= MAX_CLIENT_CIRCULAR_SIZE)
    {
        client->reply.currIndex = 0;
    }

    return retVal;
}

static RETURN_STATUS handleMsg(Client_t *cli)
{
    RETURN_STATUS retVal;
    U32 i;
    U8  replyMsg[MAX_CLIENT_REPLY_BUFF_SIZE] = {0,};
    U32 rBufLeng;

    for (i = 0; i < MAX_MSG_HANDLER_LIST_NUM; i++)
    {
        if (IS_SAFELY_PTR(gs_msgHandlerList[i].msgHandler))
        {
            retVal = gs_msgHandlerList[i].msgHandler(&cli->message[cli->rcvMsgTail], replyMsg, &rBufLeng);
            if ((SUCCESS == retVal) && (rBufLeng > 0))
            {
                retVal = sendReplyMessage(cli, replyMsg, rBufLeng);
            }
        }
    }

    return retVal;
}
static void messageHandlerTask(void * pvParameters)
{
    zosDelayTask(1000); //wait once before starting
    Client_t *client = NULL;

    while (1)
    {
        if (0 < zosMsgQueueReceive(gs_rcvMsgQue, (char *)client, sizeof(Client_t *), WAIT_10_MS))
        {
            handleMsg(client);
        }

        appTskMngImOK(gs_msgHandTaskID);
    }
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appMsgHandlerInit(void)
{
    RETURN_STATUS retVal = FAILURE;
    ZOsTaskParameters tempParam;

    /*
     * Received msg from gueue will not be handled directly. The messages will add the Message Handler
     * internal queue and they will be handled in order. With this way client will not be blocked.
     */
    gs_rcvMsgQue = zosMsgQueueCreate(QUEUE_NAME("AppMsgHandler"), sizeof(Client_t *),  MAX_MSG_BUFFER_NUM*MAX_CLINT_NUMBER);
    if (OS_INVALID_QUEUE != gs_rcvMsgQue)
    {
        tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
        tempParam.stackSize = ZOS_MIN_STACK_SIZE;

        gs_msgHandTaskID = appTskMngCreate("MSG_HND", messageHandlerTask, NULL, &tempParam, TRUE);
        if (OS_INVALID_TASK_ID != gs_msgHandTaskID)
        {
            zosSuspendTask(gs_msgHandTaskID);
            retVal = SUCCESS;
        }
    }

    return retVal;
}

RETURN_STATUS appMsgHandlerAddHandler(const char *name, MsgHandler_t msgHnd)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    for (i = 0; i < MAX_MSG_HANDLER_LIST_NUM; i++)
    {
        if (NULL == gs_msgHandlerList[i].msgHandler) //find empty place
        {
            gs_msgHandlerList[i].msgHandler = msgHnd;
            strcpy(gs_msgHandlerList[i].name, name);
            retVal = SUCCESS;
            DEBUG_DEBUG("->[I] Added message handler: %s", name);
            appLogRec(g_sysLoggerID, "Added message handler: %s", name);
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appMsgHandlerRemoveHandler(const char *name)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    for (i = 0; i < MAX_MSG_HANDLER_LIST_NUM; i++)
    {
        if (NULL != gs_msgHandlerList[i].msgHandler && strcmp(gs_msgHandlerList[i].name, name)) //find index
        {
            gs_msgHandlerList[i].msgHandler = NULL;
            memset(gs_msgHandlerList[i].name, '\0', sizeof(gs_msgHandlerList[i].name));
            retVal = SUCCESS;
            DEBUG_DEBUG("->[I] Removed message handler: %s", name);
            appLogRec(g_sysLoggerID, "Removed message handler: %s", name);
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appMsgHandlerHandleMsg(const char *cliName, Msg_Handler_Message *msg)
{
    RETURN_STATUS retVal = FAILURE;
    Client_t *client;

    if (IS_NULL_PTR(msg))
    {
        return retVal;
    }

    client = gs_Clients;

    //find the client
    while (client != NULL && (0 != strcmp(client->name, cliName)))
    {
        client = client->next;
    }

    if (client != NULL)
    {
        /* Get copy of received msg, but be careful message structure has data pointer,
         * so sender cannot free this pointer until message handling completed.
         */
        memcpy(&client->message[client->rcvMsgHead], msg, sizeof(Msg_Handler_Message));

        if (QUEUE_SUCCESS == zosMsgQueueSend(gs_rcvMsgQue, (const char *)client, sizeof(Client_t *), TIME_OUT_10MS))
        {
            retVal = SUCCESS;
        }
    }

    return retVal;
}

OsQueue appMsgHandlerAddClient(const char *cliName)
{
    OsQueue retQueNum = OS_INVALID_QUEUE;
    Client_t *client;
    Client_t *newClient;

    if (IS_NULL_PTR(cliName))
    {
        return retQueNum;
    }

    newClient = zosAllocMem(sizeof(Client_t));
    if (NULL != newClient)
    {
        newClient->next = NULL;

        if (gs_Clients == NULL)
        {
            gs_Clients = newClient;
        }
        else
        {
            while(client->next != NULL)
            {
                client = client->next; /* find end of the list to add the new client */
            }
        }

        client = newClient;

        client->que = zosMsgQueueCreate(QUEUE_NAME(cliName), MAX_CLIENT_CIRCULAR_SIZE,  MAX_CLIENT_REPLY_BUFF_SIZE);
        if (OS_INVALID_QUEUE != client->que)
        {
            strncpy(client->name, cliName, MAX_CLIENT_NAME_LENG);
            retQueNum = client->que ;
        }
        else
        {
            zosFreeMem(client);
            client = NULL;
        }
    }

    return retQueNum;
}

RETURN_STATUS appMsgHandlerRemoveClient(const char *cliName)
{
    //If the list is empty
    if (gs_Clients == NULL)
    {
        return FAILURE;
    }

    Client_t *temp = gs_Clients;

    // If the node to be deleted is the head node
    if (temp != NULL && (0 == strcmp(temp->name, cliName)))
    {
        gs_Clients = temp->next;  // Move head to the next node
        zosFreeMem(temp);  // Free the memory of the old head
        return SUCCESS;
    }

    //Search for the node to be deleted (not the head)
    Client_t *prev = NULL;
    while (temp != NULL && (0 != strcmp(temp->name, cliName)))
    {
        prev = temp;
        temp = temp->next;
    }

    //If the node to be deleted is not found
    if (temp == NULL)
    {
        //todo: add sys log and debug log
        return FAILURE;
    }

    //Unlink the node from the linked list
    prev->next = temp->next;
    zosFreeMem(temp);

    return SUCCESS;
}

RETURN_STATUS appMsgHandlerStart(void)
{
    return SUCCESS;
}

RETURN_STATUS appMsgHandlerPause(void)
{
    return SUCCESS;
}

RETURN_STATUS appMsgHandlerStop(void)
{
    return SUCCESS;
}


/******************************** End Of File *********************************/
