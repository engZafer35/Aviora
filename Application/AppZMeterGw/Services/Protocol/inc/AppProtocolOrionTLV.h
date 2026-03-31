/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 30, 2026
* #File Name    : AppProtocolOrionTLV.h
*******************************************************************************/

/******************************************************************************
* OrionTLV TLV-over-TCP protocol handler for IoT meter gateway.
*
* Wire format
* ───────────
*  '$' { TAG(2) LEN(2) VALUE(LEN) }* '#'
*
*  - Packet starts with '$' (0x24) and ends with '#' (0x23).
*  - Each field is  TAG(2 bytes, big-endian)
*                 + LEN(2 bytes, big-endian, value byte count)
*                 + VALUE(LEN bytes).
*  - Multiple TLV fields can appear in a single packet.
*  - No nesting: every tag carries exactly one piece of data.
*  - Every message carries ORION_TAG_TRANS_NUMBER for session tracking.
*    The initiator generates the number; all messages in the same session
*    reuse it.  Range 1..0xFFFF, wraps back to 1.
*
* Data encoding
* ─────────────
*  - Strings : raw ASCII, 1 byte per character (no null terminator on wire)
*  - Bool    : 1 byte  (0x01 = true, 0x00 = false)
*  - uint8   : 1 byte
*  - uint16  : 2 bytes big-endian
*  - uint32  : 4 bytes big-endian
*
* Example — Ident message (device → server), transNumber = 45:
*   $ 00FF 0002 002D                      // TRANS_NUMBER = 45
*     0001 0003 "AVI"                     // FLAG
*     0002 000F "0123456789ABCDE"         // SERIAL_NUMBER
*     0003 0001 01                        // FUNCTION = ORION_FUNC_IDENT
*     0101 0001 00                        // REGISTERED = false
*     0102 0003 "AVI"                     // DEVICE_BRAND
*     0103 0008 "AVIO2622"               // DEVICE_MODEL
*     0104 0013 "2021-06-02 17:19:58"    // DEVICE_DATE
*     0105 000D "192.168.1.10"           // PULL_IP
*     0106 0002 0A3E                      // PULL_PORT = 2622
*   #
*
* Push socket  : device → server (ident, alive, readout/loadprofile data)
* Pull socket  : server → device (log, setting, fwUpdate, readout request,
*                loadprofile request, directive CRUD)
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_PROTOCOL_ORION_TLV_H__
#define __APP_PROTOCOL_ORION_TLV_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include <stdint.h>

/******************************MACRO DEFINITIONS*******************************/

#define ORION_FLAG                    "AVI"
#define ORION_BRAND                   "AVI"
#define ORION_MODEL                   "AVIO2622"

#define ORION_PACKET_MAX_SIZE         (1024)
#define ORION_DATA_CHUNK_SIZE         (700)

#define ORION_ALIVE_INTERVAL_S        (300)     /* 5 min */
#define ORION_REGISTER_RETRY_S        (30)      /* 30 s  */
#define ORION_ACK_TIMEOUT_MS          (10000)   /* 10 s  */
#define ORION_CONNECT_TIMEOUT_MS      (10000)   /* 10 s  */

#define ORION_REGISTER_FILE           "orion_register.dat"
#define ORION_SERVER_FILE             "orion_server.dat"

#define ORION_PKT_START               '$'       /* 0x24 */
#define ORION_PKT_END                 '#'       /* 0x23 */

#define ORION_SESSION_MAX             (8)
#define ORION_SESSION_TIMEOUT         (10000)   /* 10 s  */


/* ─── TAG Definitions ─────────────────────────────────────────────────────
 *  0x00XX  Common / header
 *  0x01XX  Ident / alive
 *  0x02XX  Packet streaming
 *  0x03XX  ACK / NACK
 *  0x04XX  Log
 *  0x05XX  Meter fields
 *  0x06XX  Server config
 *  0x07XX  Readout / loadprofile
 *  0x08XX  Directive
 *  0x09XX  FW update
 *  0x0AXX  Error
 * ──────────────────────────────────────────────────────────────────────── */

/* Internal TLV tags and parser/builder types are kept in .c file. */

/* ── Event types ── */
typedef enum
{
    ORION_EVENT_ALIVE,
    ORION_EVENT_FW_UPGRADE_SUCCESS,
} OrionEventType_t;

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/* ── Protocol life-cycle ── */

/**
 * @brief  Initialise the OrionTLV protocol module.
 * @param  serialNumber  Device serial number (null-terminated, max 16 chars)
 * @param  serverIP      Default push-server IP  (null-terminated)
 * @param  serverPort    Default push-server port
 * @param  deviceIP      This device's own IP reported in ident
 * @param  pullPort      TCP-server port opened on this device (pull)
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolOrionTLVInit(const char *serialNumber,
                                     const char *serverIP, int serverPort,
                                     const char *deviceIP, int pullPort);

/**
 * @brief  Start the protocol task (creates TCP connections, enters state machine).
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolOrionTLVStart(void);

/**
 * @brief  Stop the protocol task and close TCP connections.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolOrionTLVStop(void);

/**
 * @brief   Feed raw bytes received from a TCP channel.
 * @param   channel     PUSH_TCP_SOCK_NAME or PULL_TCP_SOCK_NAME
 * @param   data        raw received bytes
 * @param   dataLength  number of bytes received
 */
void appProtocolOrionTLVPutIncomingMessage(const char *channel,
                                          const char *data,
                                          unsigned int dataLength);

/**
 * @brief   Post an asynchronous event to the protocol service.
 * @param   type   event type
 * @return  TRUE if queued, FALSE if queue full
 */
BOOL appProtocolOrionTLVSendEvent(OrionEventType_t type);

#endif /* __APP_PROTOCOL_ORION_TLV_H__ */

/********************************* End Of File ********************************/
