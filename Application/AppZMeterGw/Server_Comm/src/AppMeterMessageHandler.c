/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 6, 2024 - 1:42:21 PM
* #File Name    : AppMeterMessageHandler.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppMeterMessageHandler.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appMeterMsgHandlerSetSerialInterface(MeterSerialInterface meterInt)
{
    return SUCCESS;
}

RETURN_STATUS appMeterAddMeter(MeterTypes_t type, MeterBrands_t brand, int meterSerialNum)
{
    DEBUG_INFO("->[I] Meter Type: %d ", type);
    DEBUG_INFO("->[I] Meter Brand %d ", brand);


    return SUCCESS;
}

RETURN_STATUS appMeterMsgHandler(Msg_Handler_Message* message, U8 *replyMsg, U32 *replyMsgLeng)
{
    DEBUG_INFO("->[I] meter message handler ");

    strcpy(replyMsg, "zafer");
    *replyMsgLeng = 6;

    return SUCCESS;
}


/******************************** End Of File *********************************/
