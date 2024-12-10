/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jul 10, 2024 - 9:38:51 AM
* #File Name    : AppServerComm.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_SERVER_COMM_H__
#define __APP_SERVER_COMM_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef enum SERVER_PROTOCOL_
{
    EN_SERVER_PROTOCOL_NONE,
    EN_SERVER_PROTOCOL_TEDAS,
    EN_SERVER_PROTOCOL_VIKO,
    EN_SERVER_PROTOCOL_ZMETER,

    EN_SERVER_PROTOCOL_MAX_NUM
}SERVER_PROTOCOL;

enum MESSAGES
{
    EN_LOCAL_MSG_ERROR
};

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appServerCommInit(SERVER_PROTOCOL protocol, const char *servIP, int port);

RETURN_STATUS appServerCommStart(void);

RETURN_STATUS appServerCommStop(void);

RETURN_STATUS appServerSendPacket(enum MESSAGES msg, const char *buff, U32 buffLeng);

#endif /* __APP_SERVER_COMM_H__ */

/********************************* End Of File ********************************/
