/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 2.0
* #Date         : Mar 28, 2026 - 18:05:26 PM
* #File Name    : AppTcpConnManager.h
*******************************************************************************/

/******************************************************************************
*
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_TCP_CONN_MANAGER_H__
#define __APP_TCP_CONN_MANAGER_H__
/*********************************INCLUDES*************************************/
#include <stdbool.h>
/******************************MACRO DEFINITIONS*******************************/
#define PUSH_TCP_SOCK_NAME  "push"  //asynchronous
#define PULL_TCP_SOCK_NAME  "pull"  //synchronous

#define MAX_PULL_CLIENT_NUMBER  (1)
/*******************************TYPE DEFINITIONS ******************************/
typedef void (*IncomingMsngCb_t)(const char *channel, const char *data, unsigned int dataLength);

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   start TCP connection
 * @param   serverIP: server IP address
 * @param   serverPort: server port
 * @param   pullPort: pull port
 * @param   incomingMsngCb: incoming message callback
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
int appTcpConnManagerStart(const char *serverIP, int serverPort, int pullPort, IncomingMsngCb_t incomingMsngCb);

/**
 * @brief   stop TCP connection
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
int appTcpConnManagerStop(void);

/**
 * @brief   get any pull client exist
 * @return  if any pull client is connected, return true
 *          otherwise return false
 */
bool appTcpConnManagerAnyPullClient(void);

/**
 * @brief   get pull connection status
 * @return  if pull socket creted, return true
 *          otherwise return false
 */
bool appTcpConnManagerIsPullReady(void);

/**
 * @brief   get push connection status
 * @return  if connected to server, return true
 *          otherwise return false
 */
bool appTcpConnManagerIsConnectedPush(void);

/**
 * @brief   request connect to server.
 *          After calling this function, call the app TcpConnManagerIsConnectedPush to get result
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
int appTcpConnManagerRequestConnect(void);

/**
 * @brief   request connect to server.
 *          After calling this function, call the appTcpConnManagerIsConnectedPush to get result
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
void appTcpConnManagerRequestPushConnect(void);

/**
 * @brief   request disconnect from server.
 *          After calling this function, call the appTcpConnManagerIsConnectedPush to get result
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
void appTcpConnManagerRequestPushDisconnect(void);

/**
 * @brief   request disconnect from server.
 *          After calling this function, call the appTcpConnManagerIsConnectedPush to get result
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
int appTcpConnManagerRequestDisconnect(void);

/**
 * @brief   send data to server.
 * @param   ch: channel name
 * @param   data: data to send
 * @param   dataLeng: data length
 * @return  if everything is OK, return 0
 *          otherwise return -1
 */
int appTcpConnManagerSend(const char *ch, const char *data, unsigned int dataLeng);

#endif /* __APP_TCP_CONN_MANAGER_H__ */

/********************************* End Of File ********************************/
