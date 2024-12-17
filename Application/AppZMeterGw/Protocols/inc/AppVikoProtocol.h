/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 13, 2024 - 3:33:30 PM
* #File Name    : AppVikoProtocol.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_VIKO_PROTOCOL_H__
#define __APP_VIKO_PROTOCOL_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

#include "AppMessageHandlerManager.h"
/******************************MACRO DEFINITIONS*******************************/
#define MIKO_MSG_HANDLER_NAME       "VIKO_MSG"
/*******************************TYPE DEFINITIONS ******************************/
typedef enum VIKO_MESSAGES
{
    EN_VIKO_MSG_ALIVE,
    EN_VIKO_MSG_REGISTER,
    EN_VIKO_MSG_READOUT_REQ,
    EN_VIKO_MSG_READ_REQ,
    EN_VIKO_MSG_READOUT_REPLY,



}VIKO_MESSAGES_t;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appVikoCreateMessage(VIKO_MESSAGES_t type, U8 *buff, U32 *leng);

RETURN_STATUS appVikoMessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng);

#endif /* __APP_VIKO_PROTOCOL_H__ */

/********************************* End Of File ********************************/
