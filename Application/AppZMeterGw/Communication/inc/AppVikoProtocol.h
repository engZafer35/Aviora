/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jul 10, 2024 - 11:57:52 AM
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
/******************************MACRO DEFINITIONS*******************************/
#define PUSH_SOCKET_IP      "127.0.0.1"
#define PUSH_SOCKET_PORT    5555

#define PULL_SOCKET_IP      "127.0.0.1"
#define PULL_SOCKET_PORT    8555

#define MAX_CLIENT_NUM      (3)
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appVikoInit(const char *servIP, int port);
void appVikoTaskStart(void);
void appVikoTaskStop(void);

#endif /* __APP_VIKO_PROTOCOL_H__ */

/********************************* End Of File ********************************/
