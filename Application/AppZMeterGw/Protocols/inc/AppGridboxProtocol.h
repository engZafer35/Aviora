/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 17, 2024 - 4:26:45 PM
* #File Name    : AppGridboxProtocol.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_GRIDBOX_PROTOCOL_H__
#define __APP_GRIDBOX_PROTOCOL_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

#include "AppMessageHandlerManager.h"
/******************************MACRO DEFINITIONS*******************************/
#define GRIDBOX_MSG_HANDLER_NAME       "GBOX_MSG"
/*******************************TYPE DEFINITIONS ******************************/
typedef enum GRIDBOX_MESSAGES
{
    EN_GBOX_MSG_ALIVE,
    EN_GBOX_MSG_REGISTER,
    EN_GBOX_MSG_READOUT_REQ,
    EN_GBOX_MSG_READ_REQ,
    EN_GBOX_MSG_READOUT_REPLY,



}GRIDBOX_MESSAGES_t;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appGridboxCreateMessage(GRIDBOX_MESSAGES_t type, U8 *buff, U32 *leng);

RETURN_STATUS appGridboxMessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng);

#endif /* __APP_GRIDBOX_PROTOCOL_H__ */

/********************************* End Of File ********************************/
