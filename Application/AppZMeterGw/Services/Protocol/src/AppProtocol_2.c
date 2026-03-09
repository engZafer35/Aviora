/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 17, 2024 - 4:26:56 PM
* #File Name    : AppProtocol_2.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppProtocol_2.h"
#include "AppMeterMessageHandler.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appProtocol2CreateMessage(PROTOCOL_2_MESSAGES_t type, U8 *buff, U32 *size)
{
    RETURN_STATUS retVal = SUCCESS;

    return retVal;
}

RETURN_STATUS appProtocol2MessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng)
{
    RETURN_STATUS retVal = FAILURE;

    if (EN_MSG_TYPE_PROTOCOL_2 == message->msgType)
    {
        DEBUG_DEBUG("->[I] Protocol-2 Message Received");
        DEBUG_DEBUG_ARRAY("Protocol-2 Rcv:", message->data, message->length);
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
