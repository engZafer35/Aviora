/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 18, 2024 - 11:41:43 AM
* #File Name    : AppServerConnManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_SERVER_CONN_MANAGER_H__
#define __APP_SERVER_CONN_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "AppMessageHandlerManager.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef struct
{
    RETURN_STATUS (*conMethodOpen)(void);  /* !< open socket or init the connection method */
    RETURN_STATUS (*conMethodStart)(void); /* !< start the conn method task */
    RETURN_STATUS (*conMethodStop)(void);  /* !< stop the conn method task */
    RETURN_STATUS (*conMethodClose)(void); /* !< Close the conn method task */
}ConnMethod_t;

typedef RETURN_STATUS (*MsgHandler_t)(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng);
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
void appSrvConnManagerStartTask(void);

RETURN_STATUS appSrvConnAddMethod(const U8 *name, ConnMethod_t *connMethod);

RETURN_STATUS appSrvConnRemoveMethod (const U8 *name);


#endif /* __APP_SERVER_CONN_MANAGER_H__ */

/********************************* End Of File ********************************/
