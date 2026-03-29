/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 28, 2026
* #File Name    : AppProtocolZD.h
*******************************************************************************/

/******************************************************************************
* JSON-over-TCP protocol handler for IoT meter gateway.
*
* Push socket  : device -> server (ident, alive, readout/loadprofile data)
* Pull socket  : server -> device (log, setting, fwUpdate, readout request,
*                loadprofile request, directive CRUD)
*
* Every JSON message carries:
*   "device": { "flag": PROTOCOL_FLAG, "serialNumber": "<serial>" }
*
* Functions:
*  ident, alive, ack, nack, log, setting, fwUpdate,
*  readout, loadprofile, directiveList, directiveAdd, directiveDelete
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_PROTOCOL_ZD_H__
#define __APP_PROTOCOL_ZD_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

#define PROTOCOL_FLAG               "AVI"
#define PROTOCOL_BRAND              "AVI"
#define PROTOCOL_MODEL              "AVIO2622"

#define PROTOCOL_PACKET_MAX_SIZE    (1024)
#define PROTOCOL_DATA_CHUNK_SIZE    (700)

#define PROTOCOL_ALIVE_INTERVAL_S   (300)    /* 5 minutes */
#define PROTOCOL_REGISTER_RETRY_S   (30)     /* 30 seconds */
#define PROTOCOL_ACK_TIMEOUT_MS     (10000)  /* 10 seconds */
#define PROTOCOL_CONNECT_TIMEOUT_MS (10000)  /* 10 seconds */

#define PROTOCOL_REGISTER_FILE      "protocol_register.dat"
#define PROTOCOL_SERVER_FILE        "protocol_server.dat"

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief  Initialise the protocol module.
 * @param  serialNumber  Device serial number (null-terminated, max 16 chars)
 * @param  serverIP      Default push-server IP  (null-terminated)
 * @param  serverPort    Default push-server port
 * @param  deviceIP      This device's own IP reported in ident message
 * @param  pullPort      TCP-server port opened on this device (pull)
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolZDInit(const char *serialNumber,
                                const char *serverIP, int serverPort,
                                const char *deviceIP, int pullPort);

/**
 * @brief  Start the protocol task (creates TCP connections and enters state machine).
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolZDStart(void);

/**
 * @brief  Stop the protocol task and close TCP connections.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolZDStop(void);

/**
 * \brief   put incoming message
 * \param   channel: PUSH_TCP_SOCK_NAME or PULL_TCP_SOCK_NAME
 * \param   data: raw received bytes
 * \param   dataLength: number of bytes received
 * \return  void
 */
void appProtocolZDPutIncomingMessage(const char *channel,
                                     const char *data,
                                     unsigned int dataLength);

#endif /* __APP_PROTOCOL_ZD_H__ */

/********************************* End Of File ********************************/
