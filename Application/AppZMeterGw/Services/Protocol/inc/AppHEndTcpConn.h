/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 18, 2024 - 4:04:18 PM
* #File Name    : AppHEndTcpConn.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_HEND_TCP_CONN_H__
#define __APP_HEND_TCP_CONN_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "AppMessageHandlerManager.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
#define SOCKET_1_TYPE           "pull"
#define SOCKET_1_PORT           6666
#define SOCKET_1_MAX_CLIENT     2




/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appHEndTcpOpenServer(const char *hEndName, const char *ip, U32 port, U32 maxClient, void *packetHandlerFunc);
RETURN_STATUS appHEndTcpCloseServer(const char *hEndName);

BOOL appHEndTcpServerIsConnected(const char *hEndName);
RETURN_STATUS appHEndTcpServerSend(const char *hEndName, const void *data, size_t dataLen);


RETURN_STATUS appHEndTcpConnect(const char *hEndName, const char *ip, U32 port, void *packetHandlerFunc);
RETURN_STATUS appHEndTcpDisconnect(const char *hEndName);

BOOL appHEndTcpClientIsConnected(const char *hEndName);
RETURN_STATUS appHEndTcpClientSend(const char *hEndName, const void *data, size_t dataLen);

#endif /* __APPLICATION_APPZMETERGW_HEADEND_INC_APPHENDTCPCONN_H__ */

/********************************* End Of File ********************************/
