/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Apr 9, 2024 - 1:43:59 PM
* #File Name    : AppInternalMsgFrame.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppInternalMsgFrame.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appIntMsgParseDevMsg(const U8 *rawMsg, U32 rawMsgLeng, DevInfoMsg *msg)
{
    return SUCCESS;
}

void appIntMsgCreateDevMsg(const DevInfoMsg *dev, U8 *buff, U32 *leng)
{
#if USE_JSON_FOR_INT_MSG
#else
    memcpy(buff, dev, sizeof(DevInfoMsg));
    *leng = sizeof(DevInfoMsg);
#endif
}

RETURN_STATUS appIntMsgParseGsmMsg(const U8 *rawMsg, U32 rawMsgLeng, GsmMsg *msg)
{
    return SUCCESS;
}

void appIntMsgCreateGsmMsg(const GsmMsg *msg, U8 *buff, U32 *leng)
{
#if USE_JSON_FOR_INT_MSG
#else
    memcpy(buff, msg, sizeof(GsmMsg));
    *leng = sizeof(GsmMsg);
#endif
}

void appIntMsgCreateTaskMngMsg(const TaskMngMsg *msg, U8 *buff, U32 *leng)
{
#if USE_JSON_FOR_INT_MSG
#else
    memcpy(buff, msg, sizeof(TaskMngMsg));
    *leng = sizeof(TaskMngMsg);
#endif
}

RETURN_STATUS appIntMsgParseTaskMngMsg(const U8 *rawMsg, U32 rawMsgLeng, TaskMngMsg *msg)
{
    return SUCCESS;
}

/******************************** End Of File *********************************/
