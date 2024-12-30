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
//        strcpy(replyMsg, "#00603~L12345604000000000010262790X0.0.PX0.0.0(600065257)\
//0.2.0(V19.01)\
//0.9.1(22:51:45)\
//0.9.2(24-12-10)\
//0.9.5(2)\
//1.8.0(000000.000*kWh)\
//1.8.1(000000.000*kWh)\
//1.8.2(000000.000*kWh)\
//1.8.3(000000.000*kWh)\
//1.8.4(000000.000*kWh)\
//1.6.0(000.000*kW)(24-12-01,00:00)\
//96.1.3(23-09-07)\
//96.2.5(23-09-07)\
//96.3.12(020.000*kW)\
//5.8.0(000000.000*kVArh)\
//8.8.0(000000.000*kVArh)\
//32.7.0(000.0)\
//52.7.0(000.0)\
//72.7.0(230.8)\
//31.7.0(000.0)\
//51.7.0(000.0)\
//71.7.0(000.0)\
//14.7.0(49.9)\
//33.7.0(1.00)\
//53.7.0(1.00)\
//73.7.0(1.00)\
//F.F.0(0000000000000000000000000000000000000010001100000000000000000000)\
//F.F.1(0000000000000000000000000000000000000000000000100000000000000000)!$");
//        *replyMsgLeng = strlen(replyMsg);

        //todo: record too long reply message to file. And return the file path

        retVal = SUCCESS;
    }

    return retVal;
}
/******************************** End Of File *********************************/
