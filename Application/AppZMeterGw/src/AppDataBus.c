/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 29 Mar 2024 - 11:33:24
* #File Name    : AppDataBus.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppDataBus.h"

#include "Midd_OSPort.h"
/****************************** MACRO DEFINITIONS *****************************/
#define INVALID_CLIENT_ID   (-1)
#define MAX_PACKET_NUMBER   (2)

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

static struct
{
    DBUS_TOPICS listenTopic;
    S32 id;
#ifdef USE_FREERTOS
    QueueHandle_t bus;
#elif OS_LINUX
    OsQueue bus;
#endif

} g_clients[MAX_CLIENT_NUMBER];
/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appDBusInit(void)
{
    RETURN_STATUS retVal = SUCCESS;
    U32 i;
    for (i = 0; i < MAX_CLIENT_NUMBER; i++)
    {
        g_clients[i].id = INVALID_CLIENT_ID;
    }

    return retVal;
}

RETURN_STATUS appDBusRegister(DBUS_TOPICS listenTopic, S32 *clientID)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    for (i = 0; i < MAX_CLIENT_NUMBER; i++)
    {
        if (INVALID_CLIENT_ID == g_clients[i].id)
        {
            g_clients[i].bus = zosMsgQueueCreate(BUS_NAME("DBUS"), MAX_PACKET_NUMBER,  sizeof(DBUS_PACKET));
            if (OS_INVALID_QUEUE != g_clients[i].bus)
            {
                g_clients[i].id = i;
                *clientID = i;
                g_clients[i].listenTopic = listenTopic;

                retVal = SUCCESS;
            }
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appDBusUnregister(S32 clientID)
{
    RETURN_STATUS retVal = FAILURE;

    if (clientID < MAX_CLIENT_NUMBER)
    {
        g_clients[clientID].id = INVALID_CLIENT_ID;
        g_clients[clientID].listenTopic = EN_DBUS_TOPIC_NO;

        if (QUEUE_SUCCESS == zosMsgQueueClose(g_clients[clientID].bus))
        {
            retVal = SUCCESS;
        }
    }

    return retVal;
}

RETURN_STATUS appDBusPublish(S32 clientID, DBUS_PACKET *packet)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    if (clientID < MAX_CLIENT_NUMBER)
    {
        for (i = 0; i < MAX_CLIENT_NUMBER; i++)
        {
            if (clientID == i) continue; //skip myself

            if (INVALID_CLIENT_ID != g_clients[i].id)
            {
                if (g_clients[i].listenTopic & packet->topic) //find subscriber for published packet
                {
                    if (QUEUE_SUCCESS == zosMsgQueueSend(g_clients[i].bus, (const char *)packet, sizeof(DBUS_PACKET), TIME_OUT_10MS))
                    {
                        retVal = SUCCESS; /* if the packet could be sent to any client, we can return success. we cannot guarantee for each client */
                    }
                }
            }
        }
    }

    return retVal;
}

RETURN_STATUS appDBusReceive(S32 clientID, DBUS_PACKET *packet, U32 timeOut)
{
    RETURN_STATUS retVal = FAILURE;
    if (clientID < MAX_CLIENT_NUMBER)
    {
        if (0 < zosMsgQueueReceive(g_clients[clientID].bus, (char *)packet, sizeof(DBUS_PACKET), timeOut))
        {
            retVal = SUCCESS;
        }
    }

    return retVal;
}

RETURN_STATUS appDBusRequest(S32 myClientID, DBUS_TOPICS topic)
{
    return SUCCESS;
}
/******************************** End Of File *********************************/
