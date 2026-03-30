/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 30, 2026
* #File Name    : AppProtocolMetalix.h
*******************************************************************************/

/******************************************************************************
* Metalix TLV-over-TCP protocol handler for IoT meter gateway.
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
*  - Every message carries MTLX_TAG_TRANS_NUMBER for session tracking.
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
*     0003 0001 01                        // FUNCTION = MTLX_FUNC_IDENT
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
#ifndef __APP_PROTOCOL_METALIX_H__
#define __APP_PROTOCOL_METALIX_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include <stdint.h>

/******************************MACRO DEFINITIONS*******************************/

#define MTLX_FLAG                    "AVI"
#define MTLX_BRAND                   "AVI"
#define MTLX_MODEL                   "AVIO2622"

#define MTLX_PACKET_MAX_SIZE         (1024)
#define MTLX_DATA_CHUNK_SIZE         (700)

#define MTLX_ALIVE_INTERVAL_S        (300)     /* 5 min */
#define MTLX_REGISTER_RETRY_S        (30)      /* 30 s  */
#define MTLX_ACK_TIMEOUT_MS          (10000)   /* 10 s  */
#define MTLX_CONNECT_TIMEOUT_MS      (10000)   /* 10 s  */

#define MTLX_REGISTER_FILE           "metalix_register.dat"
#define MTLX_SERVER_FILE             "metalix_server.dat"

#define MTLX_PKT_START               '$'       /* 0x24 */
#define MTLX_PKT_END                 '#'       /* 0x23 */

#define MTLX_SESSION_MAX             (8)
#define MTLX_SESSION_RETRY_MAX       (3)

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

/* ── Common ── */
#define MTLX_TAG_FLAG                0x0001  /* string  "AVI"                   */
#define MTLX_TAG_SERIAL_NUMBER       0x0002  /* string  device serial           */
#define MTLX_TAG_FUNCTION            0x0003  /* uint8   MtlxFunction_t enum     */
#define MTLX_TAG_TRANS_NUMBER        0x00FF  /* uint16  session transaction num */

/* ── Ident / Alive ── */
#define MTLX_TAG_REGISTERED          0x0101  /* bool  device → srv (current)    */
#define MTLX_TAG_DEVICE_BRAND        0x0102  /* string  "AVI"                   */
#define MTLX_TAG_DEVICE_MODEL        0x0103  /* string  "AVIO2622"              */
#define MTLX_TAG_DEVICE_DATE         0x0104  /* string  "YYYY-MM-DD HH:MM:SS"  */
#define MTLX_TAG_PULL_IP             0x0105  /* string  device IP               */
#define MTLX_TAG_PULL_PORT           0x0106  /* uint16  pull-server port        */
#define MTLX_TAG_REGISTER            0x0107  /* bool  srv → device (register ok)*/

/* ── Packet Streaming ── */
#define MTLX_TAG_PACKET_NUM          0x0201  /* uint16  sequential packet num   */
#define MTLX_TAG_PACKET_STREAM       0x0202  /* bool    more packets follow?    */

/* ── ACK / NACK ── */
#define MTLX_TAG_ACK_STATUS          0x0301  /* bool  true=ACK  false=NACK      */

/* ── Log ── */
#define MTLX_TAG_LOG_DATA            0x0401  /* string  log text chunk          */

/* ── Meter Fields ── */
#define MTLX_TAG_METER_OPERATION     0x0501  /* string  "add" / "remove"        */
#define MTLX_TAG_METER_PROTOCOL      0x0502  /* string  "IEC62056"              */
#define MTLX_TAG_METER_TYPE          0x0503  /* string  "electricity"           */
#define MTLX_TAG_METER_BRAND         0x0504  /* string  "MKL"                   */
#define MTLX_TAG_METER_SERIAL_NUM    0x0505  /* string  meter serial            */
#define MTLX_TAG_METER_SERIAL_PORT   0x0506  /* string  "port-1"               */
#define MTLX_TAG_METER_INIT_BAUD     0x0507  /* uint32  baud rate               */
#define MTLX_TAG_METER_FIX_BAUD      0x0508  /* bool    fixed baud?             */
#define MTLX_TAG_METER_FRAME         0x0509  /* string  "7E1"                   */
#define MTLX_TAG_METER_CUSTOMER_NUM  0x050A  /* string  customer number         */
#define MTLX_TAG_METER_INDEX         0x050B  /* uint8   separator, multi-meter  */

/* ── Server Config (setting request) ── */
#define MTLX_TAG_SERVER_IP           0x0601  /* string  new server IP           */
#define MTLX_TAG_SERVER_PORT         0x0602  /* uint16  new server port         */

/* ── Readout / LoadProfile ── */
#define MTLX_TAG_METER_ID            0x0701  /* string  meter identification    */
#define MTLX_TAG_READOUT_DATA        0x0702  /* string  readout / LP data chunk */
#define MTLX_TAG_DIRECTIVE_NAME      0x0703  /* string  directive name          */
#define MTLX_TAG_START_DATE          0x0704  /* string  LP start date           */
#define MTLX_TAG_END_DATE            0x0705  /* string  LP end date             */

/* ── Directive ── */
#define MTLX_TAG_DIRECTIVE_ID        0x0801  /* string  directive ID            */
#define MTLX_TAG_DIRECTIVE_DATA      0x0802  /* string  raw directive body      */

/* ── FW Update ── */
#define MTLX_TAG_FW_ADDRESS          0x0901  /* string  FTP URL                 */

/* ── Error ── */
#define MTLX_TAG_ERROR_CODE          0x0A01  /* uint16  error code              */

/*******************************TYPE DEFINITIONS ******************************/

typedef enum
{
    MTLX_FUNC_IDENT          = 0x01,
    MTLX_FUNC_ALIVE          = 0x02,
    MTLX_FUNC_ACK            = 0x03,
    MTLX_FUNC_NACK           = 0x04,
    MTLX_FUNC_LOG            = 0x05,
    MTLX_FUNC_SETTING        = 0x06,
    MTLX_FUNC_FW_UPDATE      = 0x07,
    MTLX_FUNC_READOUT        = 0x08,
    MTLX_FUNC_LOADPROFILE    = 0x09,
    MTLX_FUNC_DIRECTIVE_LIST = 0x0A,
    MTLX_FUNC_DIRECTIVE_ADD  = 0x0B,
    MTLX_FUNC_DIRECTIVE_DEL  = 0x0C,
} MtlxFunction_t;

/* ── Packet Builder ── */
typedef struct
{
    uint8_t  *buf;
    uint16_t  capacity;
    uint16_t  pos;
    BOOL      overflow;
} MtlxBuilder_t;

/* ── Packet Parser ── */
typedef struct
{
    const uint8_t *data;
    uint16_t       length;
    uint16_t       cursor;
} MtlxParser_t;

/* ── Event types ── */
typedef enum
{
    MTLX_EVENT_ALIVE,
    MTLX_EVENT_FW_UPGRADE_SUCCESS,
} MtlxEventType_t;

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/* ── Builder API ── */
void     mtlxBuilderInit  (MtlxBuilder_t *b, uint8_t *buf, uint16_t capacity);
void     mtlxPacketBegin  (MtlxBuilder_t *b);
void     mtlxAddString    (MtlxBuilder_t *b, uint16_t tag, const char *str);
void     mtlxAddBool      (MtlxBuilder_t *b, uint16_t tag, BOOL val);
void     mtlxAddUint8     (MtlxBuilder_t *b, uint16_t tag, uint8_t val);
void     mtlxAddUint16    (MtlxBuilder_t *b, uint16_t tag, uint16_t val);
void     mtlxAddUint32    (MtlxBuilder_t *b, uint16_t tag, uint32_t val);
void     mtlxAddRaw       (MtlxBuilder_t *b, uint16_t tag, const uint8_t *data, uint16_t len);
uint16_t mtlxPacketEnd    (MtlxBuilder_t *b);

/* ── Parser API ── */
BOOL     mtlxParserInit   (MtlxParser_t *p, const uint8_t *raw, uint16_t rawLen);
void     mtlxParserReset  (MtlxParser_t *p);
BOOL     mtlxParserNext   (MtlxParser_t *p, uint16_t *tag, const uint8_t **value, uint16_t *valueLen);
BOOL     mtlxGetString    (MtlxParser_t *p, uint16_t tag, char *out, uint16_t outSz);
BOOL     mtlxGetBool      (MtlxParser_t *p, uint16_t tag, BOOL *out);
BOOL     mtlxGetUint8     (MtlxParser_t *p, uint16_t tag, uint8_t *out);
BOOL     mtlxGetUint16    (MtlxParser_t *p, uint16_t tag, uint16_t *out);
BOOL     mtlxGetUint32    (MtlxParser_t *p, uint16_t tag, uint32_t *out);

/* ── Protocol life-cycle ── */

/**
 * @brief  Initialise the Metalix protocol module.
 * @param  serialNumber  Device serial number (null-terminated, max 16 chars)
 * @param  serverIP      Default push-server IP  (null-terminated)
 * @param  serverPort    Default push-server port
 * @param  deviceIP      This device's own IP reported in ident
 * @param  pullPort      TCP-server port opened on this device (pull)
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetalixInit(const char *serialNumber,
                                     const char *serverIP, int serverPort,
                                     const char *deviceIP, int pullPort);

/**
 * @brief  Start the protocol task (creates TCP connections, enters state machine).
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetalixStart(void);

/**
 * @brief  Stop the protocol task and close TCP connections.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetalixStop(void);

/**
 * @brief   Feed raw bytes received from a TCP channel.
 * @param   channel     PUSH_TCP_SOCK_NAME or PULL_TCP_SOCK_NAME
 * @param   data        raw received bytes
 * @param   dataLength  number of bytes received
 */
void appProtocolMetalixPutIncomingMessage(const char *channel,
                                          const char *data,
                                          unsigned int dataLength);

/**
 * @brief   Post an asynchronous event to the protocol service.
 * @param   type   event type
 * @return  TRUE if queued, FALSE if queue full
 */
BOOL appProtocolMetalixSendEvent(MtlxEventType_t type);

#endif /* __APP_PROTOCOL_METALIX_H__ */

/********************************* End Of File ********************************/
