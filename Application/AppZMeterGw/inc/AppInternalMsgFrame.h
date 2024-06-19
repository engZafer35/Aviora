/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Apr 9, 2024 - 1:43:24 PM
* #File Name    : AppInternalMsgFrame.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_INTERNAL_MSG_FRAME_H___
#define __APP_INTERNAL_MSG_FRAME_H___
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "AppDataBus.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef enum
{
//    EN_WORKING_MODE_NONE,
    EN_WORKING_MODE_STARTING,   /** The device works in this mode until the battery is full. */
    EN_WORKING_MODE_MAIN,       /** AC input and SCap is ready */
    EN_WORKING_MODE_POWER_DOWN, /** AC input off */
//    EN_WORKING_MODE_SUSPEND,
//    EN_WORKING_MODE_NO_GSM_CONNECTION,
//    EN_WORKING_MODE_GSM_CONNECTION_OK,
    EN_WORKING_MODE_SW_UPDATING,
    EN_WORKING_MODE_FAILURE,

    EN_WORKING_MODE_MAX_NUMBER
}DEV_WORKING_MODE;

typedef struct
{
    U32   signalLevel;
    BOOL  modemStat;  /** connection status */
    BOOL  pppStat;    /** ppp connection status, 1: ppp OK, 0: ppp not OK */
}GsmMsg;

typedef struct
{
    DEV_WORKING_MODE wMode;
    BOOL AC;    /** 1:AC input exists, 0: doesn't */
    BOOL SCAP;  /** 1:SCAP is full */
}DevInfoMsg;

typedef struct
{
    BOOL restart;
}TaskMngMsg;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
void appIntMsgCreateDevMsg(const DevInfoMsg *dev, U8 *buff, U32 *leng);

RETURN_STATUS appIntMsgParseDevMsg(const U8 *rawMsg, U32 rawMsgLeng, DevInfoMsg *msg);

void appIntMsgCreateGsmMsg(const GsmMsg *msg, U8 *buff, U32 *leng);

RETURN_STATUS appIntMsgParseGsmMsg(const U8 *rawMsg, U32 rawMsgLeng, GsmMsg *msg);

void appIntMsgCreateTaskMngMsg(const TaskMngMsg *msg, U8 *buff, U32 *leng);

RETURN_STATUS appIntMsgParseTaskMngMsg(const U8 *rawMsg, U32 rawMsgLeng, TaskMngMsg *msg);

#endif /* __APP_INTERNAL_MSG_FRAME_H___ */

/********************************* End Of File ********************************/
