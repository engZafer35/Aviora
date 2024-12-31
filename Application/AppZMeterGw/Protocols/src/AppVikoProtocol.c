/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 13, 2024 - 3:42:39 PM
* #File Name    : AppVikoProtocol.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppVikoProtocol.h"
#include "AppMeterMessageHandler.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appVikoCreateMessage(VIKO_MESSAGES_t type, U8 *buff, U32 *size)
{
    RETURN_STATUS retVal = SUCCESS;

    return retVal;
}

RETURN_STATUS appVikoMessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng)
{
    RETURN_STATUS retVal = FAILURE;

    if (EN_MSG_TYPE_VIKO == message->msgType)
    {
        DEBUG_DEBUG("->[I] Viko Message Received");
        DEBUG_DEBUG_ARRAY("Viko Rcv:", message->data, message->length);
//        appLogRec(g_sysLoggerID, message->data);todo log hex buffer should be supported

        printf("rcv: %s \n", message->data);
        appMeterMsgHandler(NULL, replyMsg, replyMsgLeng);

        //todo: record too long reply message to file. And return the file path

        retVal = SUCCESS;
    }

    return retVal;
}
/******************************** End Of File *********************************/
