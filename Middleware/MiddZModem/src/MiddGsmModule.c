/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 26 Mar 2024 - 15:58:37
* #File Name    : MiddGsmModule.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "MiddGsmModule.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/


RETURN_STATUS middGsmInitModule(void)
{
    return SUCCESS;
}

RETURN_STATUS middGsmPPPConnection(BOOL stat) //TRUE create ppp connection, 0 close ppp connection
{
    return SUCCESS;
}

RETURN_STATUS middGsmPower(BOOL stat) //1 power up, 0 power donw
{
    return SUCCESS;
}

RETURN_STATUS middGsmSwReset(void)
{
    return SUCCESS;
}

RETURN_STATUS middGsmATCommand(const char *atMsg)
{
    return SUCCESS;
}

RETURN_STATUS middGsmGetConnStatus(void)
{
    return SUCCESS;
}

U32 middGsmGetSignalLevel(void)
{
    return 50;
}

RETURN_STATUS middGsmSetWorkingMode(EN_GSM_WORKING_MODE mode)
{
    return SUCCESS;
}

RETURN_STATUS middGsmGetWorkingMode(void)
{
    return SUCCESS;
}


/******************************** End Of File *********************************/
