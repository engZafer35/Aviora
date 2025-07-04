/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Nov 16, 2024 - 12:02:13 AM
* #File Name    : AppMessageHandlerManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_SERVER_MESSAGE_HANDLER_H__
#define __APP_SERVER_MESSAGE_HANDLER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

#include "Midd_OSPort.h"
/******************************MACRO DEFINITIONS*******************************/

#define MAX_CLINT_NUMBER  (3)

/*******************************TYPE DEFINITIONS ******************************/
typedef enum MESSAGE_TYPE
{
    EN_MSG_TYPE_PROTOCOL_2,
    EN_MSG_TYPE_AVIORA,
}MESSAGE_TYPE_T;

typedef struct
{
    char*               data;
    size_t              length;
    MESSAGE_TYPE_T      msgType;  // This can help decide which handler to use (e.g., data from meter, internal info)
} Msg_Handler_Message;

typedef RETURN_STATUS (*MsgHandler_t)(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng);

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appMsgHandlerInit(void);

RETURN_STATUS appMsgHandlerAddHandler(const char *name, MsgHandler_t msgHnd);

RETURN_STATUS appMsgHandlerRemoveHandler(const char *name);

RETURN_STATUS appMsgHandlerHandleMsg(const char *cliName, Msg_Handler_Message *msg);

OsQueue appMsgHandlerAddClient(const char *cliName);

RETURN_STATUS appMsgHandlerRemoveClient(const char *cliName);

RETURN_STATUS appMsgHandlerStart(void);

RETURN_STATUS appMsgHandlerPause(void);

RETURN_STATUS appMsgHandlerStop(void);


#endif /* __APP_SERVER_MESSAGE_HANDLER_H__ */

/********************************* End Of File ********************************/
