/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 17, 2024 - 4:26:45 PM
* #File Name    : AppProtocol_2.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_PROTOCOL_2_PROTOCOL_H__
#define __APP_PROTOCOL_2_PROTOCOL_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

#include "AppMessageHandlerManager.h"
/******************************MACRO DEFINITIONS*******************************/
#define PROTOCOL2_MSG_HANDLER_NAME       "P2_MSG"
/*******************************TYPE DEFINITIONS ******************************/
typedef enum PROTOCOL_2_MESSAGES
{
    EN_Pro2_MSG_ALIVE,
    EN_Pro2_MSG_REGISTER,


}PROTOCOL_2_MESSAGES_t;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appProtocol2CreateMessage(PROTOCOL_2_MESSAGES_t type, U8 *buff, U32 *leng);

RETURN_STATUS appProtocol2MessageHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng);

#endif /* __APP_PROTOCOL_2_PROTOCOL_H__ */

/********************************* End Of File ********************************/
