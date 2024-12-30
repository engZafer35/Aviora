/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 17, 2024 - 4:26:56 PM
* #File Name    : AppGridboxProtocol.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppGridboxProtocol.h"
#include "AppMeterMessageHandler.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appGridboxCreateMessage(GRIDBOX_MESSAGES_t type, U8 *buff, U32 *size)
{
    RETURN_STATUS retVal = SUCCESS;

    return retVal;
}

RETURN_STATUS appGridboxMessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng)
{
    RETURN_STATUS retVal = FAILURE;

    if (EN_MSG_TYPE_GRIDBOX == message->msgType)
    {
        DEBUG_DEBUG("->[I] Gridbox Message Received");
        DEBUG_DEBUG_ARRAY("Gridbox Rcv:", message->data, message->length);
//        appLogRec(g_sysLoggerID, message->data);todo log hex buffer should be supported

        printf("rcv: %s \n", message->data);
        appMeterMsgHandler(NULL, replyMsg, replyMsgLeng);
//        strcpy(replyMsg, "#OK$");
//        *replyMsgLeng = strlen(replyMsg) +1;

        //todo: record too long reply message to file. And return the file path

        retVal = SUCCESS;
    }

    return retVal;
}
/******************************** End Of File *********************************/
