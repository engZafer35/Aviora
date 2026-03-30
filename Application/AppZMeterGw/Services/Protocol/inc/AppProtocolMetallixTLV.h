/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 29, 2026
* #File Name    : AppProtocolMetallixTLV.h
*******************************************************************************/

/******************************************************************************
* Metallix TLV-over-TCP protocol handler for IoT meter gateway.
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
*
* Data encoding
* ─────────────
*  - Strings : raw ASCII, 1 byte per character (no null terminator on wire)
*  - Bool    : 1 byte  (0x01 = true, 0x00 = false)
*  - uint8   : 1 byte
*  - uint16  : 2 bytes big-endian
*  - uint32  : 4 bytes big-endian
*  - int16   : 2 bytes big-endian (two's complement)
*
* Example
* ───────
*  Ident message (device → server):
*   $  0001 0003 "AVI"                   // FLAG
*      0002 000F "0123456789ABCDE"       // SERIAL_NUMBER
*      0003 0001 01                      // FUNCTION = FUNC_IDENT
*      0101 0001 00                      // REGISTERED = false
*      0102 0003 "AVI"                   // DEVICE_BRAND
*      0103 0008 "AVIO2622"             // DEVICE_MODEL
*      0104 0013 "2021-06-02 17:19:58"  // DEVICE_DATE
*      0105 000C "192.168.1.10"         // PULL_IP
*      0106 0002 0A3E                    // PULL_PORT = 2622
*   #
*
* Push socket  : device → server (ident, alive, readout/loadprofile data)
* Pull socket  : server → device (log, setting, fwUpdate, readout request,
*                loadprofile request, directive CRUD)
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_PROTOCOL_METALLIX_TLV_H__
#define __APP_PROTOCOL_METALLIX_TLV_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include <stdint.h>

/******************************MACRO DEFINITIONS*******************************/

#define PROTO_METLLX_FLAG                "AVI"
#define PROTO_METLLX_BRAND               "AVI"
#define PROTO_METLLX_MODEL               "AVIO2622"

#define PROTO_METLLX_PACKET_MAX_SIZE     (1024)
#define PROTO_METLLX_DATA_CHUNK_SIZE     (700)

#define PROTO_METLLX_ALIVE_INTERVAL_S    (300)     /* 5 min */
#define PROTO_METLLX_REGISTER_RETRY_S    (30)      /* 30 s  */
#define PROTO_METLLX_ACK_TIMEOUT_MS      (10000)   /* 10 s  */
#define PROTO_METLLX_CONNECT_TIMEOUT_MS  (10000)   /* 10 s  */

#define PROTO_METLLX_REGISTER_FILE       "metallixtlv_register.dat"
#define PROTO_METLLX_SERVER_FILE         "metallixtlv_server.dat"

#define PROTO_METLLX_PKT_START           '$'       /* 0x24 */
#define PROTO_METLLX_PKT_END             '#'       /* 0x23 */

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
#define TAG_FLAG                0x0001  /* string  "AVI"                      */
#define TAG_SERIAL_NUMBER       0x0002  /* string  device serial              */
#define TAG_FUNCTION            0x0003  /* uint8   MetallixTLV_Function_t enum    */

/* ── Ident / Alive ── */
#define TAG_REGISTERED          0x0101  /* bool    device → srv (current)     */
#define TAG_DEVICE_BRAND        0x0102  /* string  "AVI"                      */
#define TAG_DEVICE_MODEL        0x0103  /* string  "AVIO2622"                 */
#define TAG_DEVICE_DATE         0x0104  /* string  "YYYY-MM-DD HH:MM:SS"     */
#define TAG_PULL_IP             0x0105  /* string  device IP                  */
#define TAG_PULL_PORT           0x0106  /* uint16  pull-server port           */
#define TAG_REGISTER            0x0107  /* bool    srv → device (register ok) */

/* ── Packet Streaming ── */
#define TAG_PACKET_NUM          0x0201  /* uint16  sequential packet number   */
#define TAG_PACKET_STREAM       0x0202  /* bool    more packets follow?       */

/* ── ACK / NACK ── */
#define TAG_ACK_STATUS          0x0301  /* bool    true=ACK  false=NACK       */

/* ── Log ── */
#define TAG_LOG_DATA            0x0401  /* string  log text chunk             */

/* ── Meter Fields ── */
#define TAG_METER_OPERATION     0x0501  /* string  "add" / "remove"           */
#define TAG_METER_PROTOCOL      0x0502  /* string  "IEC62056"                 */
#define TAG_METER_TYPE          0x0503  /* string  "electricity"              */
#define TAG_METER_BRAND         0x0504  /* string  "MKL"                      */
#define TAG_METER_SERIAL_NUM    0x0505  /* string  meter serial               */
#define TAG_METER_SERIAL_PORT   0x0506  /* string  "port-1"                   */
#define TAG_METER_INIT_BAUD     0x0507  /* uint32  baud rate                  */
#define TAG_METER_FIX_BAUD      0x0508  /* bool    fixed baud?                */
#define TAG_METER_FRAME         0x0509  /* string  "7E1"                      */
#define TAG_METER_CUSTOMER_NUM  0x050A  /* string  customer number            */
#define TAG_METER_INDEX         0x050B  /* uint8   separator for multi-meter  */

/* ── Server Config (setting request) ── */
#define TAG_SERVER_IP           0x0601  /* string  new server IP              */
#define TAG_SERVER_PORT         0x0602  /* uint16  new server port            */

/* ── Readout / LoadProfile ── */
#define TAG_METER_ID            0x0701  /* string  meter identification       */
#define TAG_READOUT_DATA        0x0702  /* string  readout / LP data chunk    */
#define TAG_DIRECTIVE_NAME      0x0703  /* string  directive name in request  */
#define TAG_START_DATE          0x0704  /* string  load-profile start date    */
#define TAG_END_DATE            0x0705  /* string  load-profile end date      */

/* ── Directive ── */
#define TAG_DIRECTIVE_ID        0x0801  /* string  directive ID               */
#define TAG_DIRECTIVE_DATA      0x0802  /* string  raw directive body         */

/* ── FW Update ── */
#define TAG_FW_ADDRESS          0x0901  /* string  FTP URL                    */

/* ── Error ── */
#define TAG_ERROR_CODE          0x0A01  /* int16   ERR_CODE_T value           */

/*******************************TYPE DEFINITIONS ******************************/

/* Message function types (value carried by TAG_FUNCTION) */
typedef enum
{
    FUNC_IDENT           = 0x01,
    FUNC_ALIVE           = 0x02,
    FUNC_ACK             = 0x03,
    FUNC_NACK            = 0x04,
    FUNC_LOG             = 0x05,
    FUNC_SETTING         = 0x06,
    FUNC_FW_UPDATE       = 0x07,
    FUNC_READOUT         = 0x08,
    FUNC_LOADPROFILE     = 0x09,
    FUNC_DIRECTIVE_LIST  = 0x0A,
    FUNC_DIRECTIVE_ADD   = 0x0B,
    FUNC_DIRECTIVE_DEL   = 0x0C,
} MetallixTLV_Function_t;

/* ── Packet Builder ────────────────────────────────────────────────────── */

typedef struct
{
    uint8_t  *buf;
    uint16_t  capacity;
    uint16_t  pos;
    BOOL      overflow;
} MetallixTLV_Builder_t;

/* ── Packet Parser (iterates TLV fields inside a received packet) ────── */

typedef struct
{
    const uint8_t *data;    /* points after '$' */
    uint16_t       length;  /* bytes between '$' and '#' */
    uint16_t       cursor;
} MetallixTLV_Parser_t;

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/* ── Builder API ── */
void     metallixTLV_BuilderInit  (MetallixTLV_Builder_t *b, uint8_t *buf, uint16_t capacity);
void     metallixTLV_PacketBegin  (MetallixTLV_Builder_t *b);
void     metallixTLV_AddString    (MetallixTLV_Builder_t *b, uint16_t tag, const char *str);
void     metallixTLV_AddStringN   (MetallixTLV_Builder_t *b, uint16_t tag, const char *str, uint16_t len);
void     metallixTLV_AddBool      (MetallixTLV_Builder_t *b, uint16_t tag, BOOL val);
void     metallixTLV_AddUint8     (MetallixTLV_Builder_t *b, uint16_t tag, uint8_t val);
void     metallixTLV_AddUint16    (MetallixTLV_Builder_t *b, uint16_t tag, uint16_t val);
void     metallixTLV_AddUint32    (MetallixTLV_Builder_t *b, uint16_t tag, uint32_t val);
void     metallixTLV_AddInt16     (MetallixTLV_Builder_t *b, uint16_t tag, int16_t val);
void     metallixTLV_AddRaw       (MetallixTLV_Builder_t *b, uint16_t tag, const uint8_t *data, uint16_t len);
uint16_t metallixTLV_PacketEnd    (MetallixTLV_Builder_t *b);
void     metallixTLV_AddDeviceHeader(MetallixTLV_Builder_t *b);

/* ── Parser API ── */
BOOL     metallixTLV_ParserInit   (MetallixTLV_Parser_t *p, const uint8_t *raw, uint16_t rawLen);
void     metallixTLV_ParserReset  (MetallixTLV_Parser_t *p);
BOOL     metallixTLV_ParserNext   (MetallixTLV_Parser_t *p, uint16_t *tag, const uint8_t **value, uint16_t *valueLen);
BOOL     metallixTLV_GetString    (MetallixTLV_Parser_t *p, uint16_t tag, char *out, uint16_t outSz);
BOOL     metallixTLV_GetBool      (MetallixTLV_Parser_t *p, uint16_t tag, BOOL *out);
BOOL     metallixTLV_GetUint8     (MetallixTLV_Parser_t *p, uint16_t tag, uint8_t *out);
BOOL     metallixTLV_GetUint16    (MetallixTLV_Parser_t *p, uint16_t tag, uint16_t *out);
BOOL     metallixTLV_GetUint32    (MetallixTLV_Parser_t *p, uint16_t tag, uint32_t *out);
BOOL     metallixTLV_GetInt16     (MetallixTLV_Parser_t *p, uint16_t tag, int16_t *out);
BOOL     metallixTLV_GetFunction  (MetallixTLV_Parser_t *p, MetallixTLV_Function_t *out);

/* ── Protocol life-cycle ── */

/**
 * @brief  Initialise the MetallixTLV protocol module.
 * @param  serialNumber  Device serial number (null-terminated, max 16 chars)
 * @param  serverIP      Default push-server IP  (null-terminated)
 * @param  serverPort    Default push-server port
 * @param  deviceIP      This device's own IP reported in ident
 * @param  pullPort      TCP-server port opened on this device (pull)
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetallixTLVInit(const char *serialNumber,
                                const char *serverIP, int serverPort,
                                const char *deviceIP, int pullPort);

/**
 * @brief  Start the protocol task (creates TCP connections, enters state machine).
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetallixTLVStart(void);

/**
 * @brief  Stop the protocol task and close TCP connections.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolMetallixTLVStop(void);

/**
 * @brief   Feed raw bytes received from a TCP channel.
 * @param   channel     PUSH_TCP_SOCK_NAME or PULL_TCP_SOCK_NAME
 * @param   data        raw received bytes
 * @param   dataLength  number of bytes received
 */
void appProtocolMetallixTLVPutIncomingMessage(const char *channel,
                                     const char *data,
                                     unsigned int dataLength);

#endif /* __APP_PROTOCOL_METALLIX_TLV_H__ */

/********************************* End Of File ********************************/
