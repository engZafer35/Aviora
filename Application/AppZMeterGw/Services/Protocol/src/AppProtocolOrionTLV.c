/******************************************************************************
* #Author       : Zafer Satilmis
* #Hhype-man    : Radical Noise - Angry Son
* #Revision     : 1.0
* #Date         : Mar 30, 2026
* #File Name    : AppProtocolOrionTLV.c
*******************************************************************************/
/******************************************************************************
* OrionTLV TLV-over-TCP protocol service with session management.
*
* Architecture :
*   - Every operation (ident, alive, pull-request, meter-data delivery) is
*     tracked as a Session with a step-based state machine.
*   - Each session carries a transactionNumber (uint16, 1..0xFFFF) so that
*     concurrent request / response pairs can be correlated.
*   - A 1-second periodic timer (MiddEventTimer) drives session timeouts
*     and the alive heartbeat counter.
*   - An event ring buffer allows asynchronous event delivery from ISRs or
*     other tasks (power events, FW upgrade results, etc.).
*   - The protocol task main loop polls RX buffers, events, and iterates
*     active sessions each cycle.
******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)

/********************************* INCLUDES ***********************************/
#include "AppProtocolOrionTLV.h"
#include "AppProtocol.h"
#include "MiddEventTimer.h"
#include "AppTcpConnManager.h"

#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppTimeService.h"
#include "AppSWUpdate.h"
#include "fs_port.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/****************************** MACRO DEFINITIONS *****************************/
#define ORION_TASK_STACK_SIZE    (4096U)
#define ORION_TASK_NAME          "OrionTLV"

#define MAX_PENDING_JOBS        (4)
#define MAX_PATH_LEN            (96)

#define FTP_FIELD_MAX           (64)
#define FTP_DEFAULT_PORT        (21)
#define FW_LOCAL_PATH           "/tmp/firmware.bin"

#define ORION_TLV_HEADER_SIZE   (4)   /* 2 tag + 2 len */

#define ORION_EVENT_Q_DEPTH     (8)

#define SFUNC_PUSH_IDENT        (0xF0)
#define SFUNC_ALIVE             (0xF1)
#define SFUNC_PUSH_METER_DATA   (0xF2)

#define ORION_PKT_START '$'       /* 0x24 */
#define ORION_PKT_END   '#'       /* 0x23 */
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
#define ORION_TAG_FLAG                0x0001
#define ORION_TAG_SERIAL_NUMBER       0x0002
#define ORION_TAG_FUNCTION            0x0003
#define ORION_TAG_TRANS_NUMBER        0x00FF

/* ── Ident / Alive ── */
#define ORION_TAG_REGISTERED          0x0101
#define ORION_TAG_DEVICE_BRAND        0x0102
#define ORION_TAG_DEVICE_MODEL        0x0103
#define ORION_TAG_DEVICE_DATE         0x0104
#define ORION_TAG_PULL_IP             0x0105
#define ORION_TAG_PULL_PORT           0x0106
#define ORION_TAG_REGISTER            0x0107

/* ── Packet Streaming ── */
#define ORION_TAG_PACKET_NUM          0x0201
#define ORION_TAG_PACKET_STREAM       0x0202

/* ── ACK / NACK ── */
#define ORION_TAG_ACK_STATUS          0x0301

/* ── Log ── */
#define ORION_TAG_LOG_DATA            0x0401

/* ── Meter Fields ── */
#define ORION_TAG_METER_OPERATION     0x0501
#define ORION_TAG_METER_PROTOCOL      0x0502
#define ORION_TAG_METER_TYPE          0x0503
#define ORION_TAG_METER_BRAND         0x0504
#define ORION_TAG_METER_SERIAL_NUM    0x0505
#define ORION_TAG_METER_SERIAL_PORT   0x0506
#define ORION_TAG_METER_INIT_BAUD     0x0507
#define ORION_TAG_METER_FIX_BAUD      0x0508
#define ORION_TAG_METER_FRAME         0x0509
#define ORION_TAG_METER_CUSTOMER_NUM  0x050A
#define ORION_TAG_METER_INDEX         0x050B

/* ── Server Config ── */
#define ORION_TAG_SERVER_IP           0x0601
#define ORION_TAG_SERVER_PORT         0x0602

/* ── Readout / LoadProfile ── */
#define ORION_TAG_METER_ID            0x0701
#define ORION_TAG_READOUT_DATA        0x0702
#define ORION_TAG_DIRECTIVE_NAME      0x0703
#define ORION_TAG_START_DATE          0x0704
#define ORION_TAG_END_DATE            0x0705

/* ── Directive ── */
#define ORION_TAG_DIRECTIVE_ID        0x0801
#define ORION_TAG_DIRECTIVE_DATA      0x0802

/* ── FW Update ── */
#define ORION_TAG_FW_ADDRESS          0x0901

/* ── Error ── */
#define ORION_TAG_ERROR_CODE          0x0A01

/******************************* TYPE DEFINITIONS *****************************/

typedef enum
{
    ORION_FUNC_IDENT          = 0x01,
    ORION_FUNC_ALIVE          = 0x02,
    ORION_FUNC_ACK            = 0x03,
    ORION_FUNC_NACK           = 0x04,
    ORION_FUNC_LOG            = 0x05,
    ORION_FUNC_SETTING        = 0x06,
    ORION_FUNC_FW_UPDATE      = 0x07,
    ORION_FUNC_READOUT        = 0x08,
    ORION_FUNC_LOADPROFILE    = 0x09,
    ORION_FUNC_DIRECTIVE_LIST = 0x0A,
    ORION_FUNC_DIRECTIVE_ADD  = 0x0B,
    ORION_FUNC_DIRECTIVE_DEL  = 0x0C,
} OrionFunction_t;

typedef struct
{
    uint8_t  *buf;
    uint16_t  capacity;
    uint16_t  pos;
    BOOL      overflow;
} OrionBuilder_t;

typedef struct
{
    const uint8_t *data;
    uint16_t       length;
    uint16_t       cursor;
} OrionParser_t;

typedef struct
{
    char     ch[16];
    uint8_t  payload[ORION_PACKET_MAX_SIZE];
    uint16_t length;
} OrionIncomingMsg_t;

typedef struct
{
    S32 taskId;
    volatile BOOL completed;
    volatile ERR_CODE_T result;
    BOOL isLoadProfile;
} PendingJob_t;

typedef struct
{
    char ip[20];
    int  port;
    int  pullPort;
} ConnConfig_t;

typedef enum
{
    SESSION_DONE = 0,
    SESSION_ERROR,
    SESSION_TIMEOUT,
} OrionTLVState_t;

typedef struct OrionSession_s OrionSession_t;

typedef void (*OrionSessionCompleteCb)(OrionSession_t *session, OrionTLVState_t state, void *arg);

struct OrionSession_s
{
    BOOL        inUse;
    uint8_t     function;
    char        channel[16];
    uint16_t    transNumber;

    uint32_t    step;
    uint32_t    timeCnt;

    uint8_t     rxBuf[ORION_PACKET_MAX_SIZE];
    uint16_t    rxLen;
    volatile BOOL rxReady;

    int         pendingJobIdx;
    OrionSessionCompleteCb onComplateFunc;
};

typedef struct
{
    OrionEventType_t type;
} OrionEventMsg_t;

/********************************** VARIABLES *********************************/

static union FlagList
{
#define MAX_INTERNAL_MAX_TASK_NUMBER    (32)  //each bit of unsigned int will be used as a task flag. so max 32 task for now

    unsigned int listAll;
    struct
    {
        unsigned int sendIdent  : 1;
        unsigned int sendAlive  : 1;

        unsigned int tcpPushInUse   : 1;

        /***************************************/
        unsigned int connType :2;  // 0 no connection, 1 is tcp, 2 is mqtt
        #define CONN_TYPE_NONE (0)
        #define CONN_TYPE_MQTT (1)
        #define CONN_TYPE_TCP  (2)
        /***************************************/

    };
}gsFlagList;

static char gs_serialNumber[20];
static char gs_serverIP[20];
static int  gs_serverPort;
static char gs_deviceIP[20];
static int  gs_pullPort;

static BOOL gs_registered = FALSE;
static OsTaskId gs_taskId = OS_INVALID_TASK_ID;

static PendingJob_t gs_pendingJobs[MAX_PENDING_JOBS];

static uint8_t gs_txBuf[ORION_PACKET_MAX_SIZE];

static OrionSession_t gs_sessionTable[ORION_SESSION_MAX];

/* Incoming message ring buffer (replaces single gs_pushRx/gs_pullRx buffers) */
#define ORION_INMSG_Q_DEPTH   (8)
static OrionIncomingMsg_t gs_inQ[ORION_INMSG_Q_DEPTH];
static volatile uint8_t gs_inHead;
static volatile uint8_t gs_inTail;
static volatile uint8_t gs_inCount;

static OrionEventMsg_t gs_eventQ[ORION_EVENT_Q_DEPTH];
static uint8_t gs_evHead;
static uint8_t gs_evTail;
static uint8_t gs_evCount;

static S32 gs_periodicTimerId = -1;

static volatile uint32_t gs_aliveCnt = 0;

static uint16_t gs_localTransNum = 0;

/* ================================================================== */
/*                       UTILITY  HELPERS                             */
/* ================================================================== */

static void writeU16BE(uint8_t *dst, uint16_t val)
{
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)(val & 0xFF);
}

static uint16_t readU16BE(const uint8_t *src)
{
    return (uint16_t)((uint16_t)src[0] << 8 | src[1]);
}

static uint16_t nextTransNumber(void)
{
    gs_localTransNum++;
    if (gs_localTransNum == 0) gs_localTransNum = 1;
    return gs_localTransNum;
}

static BOOL inQueuePush(const char *ch, const uint8_t *payload, uint16_t len)
{
    if (payload == NULL || len == 0)
        return FALSE;

    if (len > ORION_PACKET_MAX_SIZE)
        len = ORION_PACKET_MAX_SIZE;

    if (gs_inCount >= ORION_INMSG_Q_DEPTH)
        return FALSE;

    OrionIncomingMsg_t *it = &gs_inQ[gs_inTail];
    memset(it, 0, sizeof(*it));
    if (ch != NULL)
    {
        strncpy(it->ch, ch, sizeof(it->ch) - 1);
        it->ch[sizeof(it->ch) - 1] = '\0';
    }
    memcpy(it->payload, payload, len);
    it->length = len;

    gs_inTail = (uint8_t)((gs_inTail + 1U) % ORION_INMSG_Q_DEPTH);
    gs_inCount++;
    return TRUE;
}

static BOOL inQueuePop(OrionIncomingMsg_t *out)
{
    if (out == NULL || gs_inCount == 0U)
        return FALSE;

    *out = gs_inQ[gs_inHead];
    gs_inHead = (uint8_t)((gs_inHead + 1U) % ORION_INMSG_Q_DEPTH);
    gs_inCount--;
    return TRUE;
}

/* ================================================================== */
/*                       TLV BUILDER                                  */
/* ================================================================== */

static void mtlxBuilderInit(OrionBuilder_t *b, uint8_t *buf, uint16_t capacity)
{
    b->buf      = buf;
    b->capacity = capacity;
    b->pos      = 0;
    b->overflow = FALSE;
}

static void mtlxPacketBegin(OrionBuilder_t *b)
{
    b->pos      = 0;
    b->overflow = FALSE;
    if (b->capacity < 2) { b->overflow = TRUE; return; }
    b->buf[b->pos++] = ORION_PKT_START;
}

static void builderAddTLV(OrionBuilder_t *b, uint16_t tag, const uint8_t *val, uint16_t valLen)
{
    if (b->overflow) return;
    if ((uint32_t)b->pos + ORION_TLV_HEADER_SIZE + valLen + 1 > b->capacity)
    {
        b->overflow = TRUE;
        return;
    }
    writeU16BE(b->buf + b->pos, tag);     b->pos += 2;
    writeU16BE(b->buf + b->pos, valLen);  b->pos += 2;
    if (valLen > 0)
    {
        memcpy(b->buf + b->pos, val, valLen);
        b->pos += valLen;
    }
}

static void mtlxAddString(OrionBuilder_t *b, uint16_t tag, const char *str)
{
    uint16_t len = (str != NULL) ? (uint16_t)strlen(str) : 0;
    builderAddTLV(b, tag, (const uint8_t *)str, len);
}

static void mtlxAddBool(OrionBuilder_t *b, uint16_t tag, BOOL val)
{
    uint8_t v = val ? 0x01 : 0x00;
    builderAddTLV(b, tag, &v, 1);
}

static void mtlxAddUint8(OrionBuilder_t *b, uint16_t tag, uint8_t val)
{
    builderAddTLV(b, tag, &val, 1);
}

static void mtlxAddUint16(OrionBuilder_t *b, uint16_t tag, uint16_t val)
{
    uint8_t tmp[2];
    writeU16BE(tmp, val);
    builderAddTLV(b, tag, tmp, 2);
}

static void mtlxAddUint32(OrionBuilder_t *b, uint16_t tag, uint32_t val)
{
    uint8_t tmp[4];
    tmp[0] = (uint8_t)(val >> 24);
    tmp[1] = (uint8_t)(val >> 16);
    tmp[2] = (uint8_t)(val >> 8);
    tmp[3] = (uint8_t)(val);
    builderAddTLV(b, tag, tmp, 4);
}

static void mtlxAddRaw(OrionBuilder_t *b, uint16_t tag, const uint8_t *data, uint16_t len)
{
    builderAddTLV(b, tag, data, len);
}

static uint16_t mtlxPacketEnd(OrionBuilder_t *b)
{
    if (b->overflow) return 0;
    if (b->pos >= b->capacity) { b->overflow = TRUE; return 0; }
    b->buf[b->pos++] = ORION_PKT_END;
    return b->pos;
}

/* ================================================================== */
/*                         TLV PARSER                                 */
/* ================================================================== */

static BOOL mtlxParserInit(OrionParser_t *p, const uint8_t *raw, uint16_t rawLen)
{
    if (raw == NULL || rawLen < 2) return FALSE;

    uint16_t start = 0;
    while (start < rawLen && raw[start] != ORION_PKT_START)
        start++;
    if (start >= rawLen) return FALSE;
    start++;

    uint16_t end = rawLen;
    for (uint16_t i = end; i > start; i--)
    {
        if (raw[i - 1] == ORION_PKT_END) { end = i - 1; break; }
    }
    if (end <= start) return FALSE;

    p->data   = raw + start;
    p->length = (uint16_t)(end - start);
    p->cursor = 0;
    return TRUE;
}

static void orionParserReset(OrionParser_t *p)
{
    p->cursor = 0;
}

static BOOL orionParserNext(OrionParser_t *p, uint16_t *tag, const uint8_t **value, uint16_t *valueLen)
{
    if (p->cursor + ORION_TLV_HEADER_SIZE > p->length) return FALSE;
    *tag      = readU16BE(p->data + p->cursor);       p->cursor += 2;
    *valueLen = readU16BE(p->data + p->cursor);       p->cursor += 2;
    if (p->cursor + *valueLen > p->length) { p->cursor = p->length; return FALSE; }
    *value = p->data + p->cursor;
    p->cursor += *valueLen;
    return TRUE;
}

static BOOL findTag(OrionParser_t *p, uint16_t wantTag, const uint8_t **value, uint16_t *valueLen)
{
    uint16_t saved = p->cursor;
    p->cursor = 0;

    uint16_t tag; const uint8_t *v; uint16_t vl;
    while (orionParserNext(p, &tag, &v, &vl))
    {
        if (tag == wantTag)
        {
            if (value)    *value    = v;
            if (valueLen) *valueLen = vl;
            p->cursor = saved;
            return TRUE;
        }
    }
    p->cursor = saved;
    return FALSE;
}

static BOOL mtlxGetString(OrionParser_t *p, uint16_t tag, char *out, uint16_t outSz)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl)) return FALSE;
    uint16_t cp = (vl < outSz - 1) ? vl : (uint16_t)(outSz - 1);
    memcpy(out, v, cp);
    out[cp] = '\0';
    return TRUE;
}

static BOOL mtlxGetBool(OrionParser_t *p, uint16_t tag, BOOL *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = (v[0] != 0) ? TRUE : FALSE;
    return TRUE;
}

static BOOL mtlxGetUint8(OrionParser_t *p, uint16_t tag, uint8_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = v[0];
    return TRUE;
}

static BOOL mtlxGetUint16(OrionParser_t *p, uint16_t tag, uint16_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 2) return FALSE;
    *out = readU16BE(v);
    return TRUE;
}

static BOOL mtlxGetUint32(OrionParser_t *p, uint16_t tag, uint32_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 4) return FALSE;
    *out = ((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
           ((uint32_t)v[2] << 8)  | v[3];
    return TRUE;
}

/* ================================================================== */
/*                   Register-state persistence                       */
/* ================================================================== */

static BOOL registerStateLoad(void)
{
    FsFile *f = fsOpenFile((char *)ORION_REGISTER_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FALSE;

    U8 val = 0; size_t got = 0;
    (void)fsReadFile(f, &val, 1, &got);
    fsCloseFile(f);
    return (got == 1 && val == 1) ? TRUE : FALSE;
}

static void registerStateSave(BOOL reg)
{
    FsFile *f = fsOpenFile((char *)ORION_REGISTER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f != NULL)
    {
        U8 val = (U8)reg;
        fsWriteFile(f, &val, 1);
        fsCloseFile(f);
    }
}

/* ================================================================== */
/*                   Server-address persistence                       */
/* ================================================================== */

static void connConfigDefault(void)
{
    strncpy(gs_serverIP, ORION_TLV_DEFAULT_SERVER_IP, sizeof(gs_serverIP) - 1);
    gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
    gs_serverPort = ORION_TLV_DEFAULT_PUSH_PORT;
    gs_pullPort = ORION_TLV_DEFAULT_PULL_PORT;
}

static RETURN_STATUS connConfigLoad(void)
{
    ConnConfig_t cfg;
    FsFile *f = fsOpenFile((char *)ORION_SERVER_FILE, FS_FILE_MODE_READ);
    if (NULL == f)
     return FAILURE;
    
     size_t got = 0;
    (void)fsReadFile(f, &cfg, sizeof(cfg), &got);
    fsCloseFile(f);

    if (got != sizeof(cfg))
        return FAILURE;

    strncpy(gs_serverIP, cfg.ip, sizeof(gs_serverIP) - 1);
    gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
    gs_serverPort = cfg.port;
    gs_pullPort = cfg.pullPort;

    return SUCCESS;
}

static RETURN_STATUS connConfigSave(void)
{
    RETURN_STATUS retVal = FAILURE;
    ConnConfig_t cfg;

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ip, gs_serverIP, sizeof(cfg.ip) - 1);
    cfg.port = gs_serverPort;
    cfg.pullPort = gs_pullPort;

    FsFile *f = fsOpenFile((char *)ORION_SERVER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (NULL != f)
    {
        if (NO_ERROR == fsWriteFile(f, &cfg, sizeof(cfg)))
        {
            retVal = SUCCESS;
        }
        fsCloseFile(f); 
    }

    return retVal;
}

/* ================================================================== */
/*                          Time helper                               */
/* ================================================================== */

static void getDeviceDateStr(char *buf, size_t bufSz)
{
    struct tm t;
    if (SUCCESS == appTimeServiceGetTime(&t))
    {
        snprintf(buf, bufSz, "%04d-%02d-%02d %02d:%02d:%02d",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec);
    }
    else
    {
        strncpy(buf, "0000-00-00 00:00:00", bufSz - 1);
        buf[bufSz - 1] = '\0';
    }
}

/* ================================================================== */
/*                    Packet builder helpers                           */
/* ================================================================== */

static void addPacketHeader(OrionBuilder_t *b, uint16_t transNum, uint8_t func)
{
    mtlxAddUint16(b, ORION_TAG_TRANS_NUMBER, transNum);
    mtlxAddString(b, ORION_TAG_FLAG, ORION_FLAG);
    mtlxAddString(b, ORION_TAG_SERIAL_NUMBER, gs_serialNumber);
    mtlxAddUint8(b, ORION_TAG_FUNCTION, func);
}

static uint16_t buildAckNack(uint8_t *buf, uint16_t sz, BOOL isAck, uint16_t transNum)
{
    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, isAck ? ORION_FUNC_ACK : ORION_FUNC_NACK);
    mtlxAddBool(&b, ORION_TAG_ACK_STATUS, isAck);
    return mtlxPacketEnd(&b);
}

static uint16_t buildIdentMsg(uint8_t *buf, uint16_t sz, uint16_t transNum)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, ORION_FUNC_IDENT);
    mtlxAddBool(&b, ORION_TAG_REGISTERED, gs_registered);
    mtlxAddString(&b, ORION_TAG_DEVICE_BRAND, ORION_BRAND);
    mtlxAddString(&b, ORION_TAG_DEVICE_MODEL, ORION_MODEL);
    mtlxAddString(&b, ORION_TAG_DEVICE_DATE, dateStr);
    mtlxAddString(&b, ORION_TAG_PULL_IP, gs_deviceIP);
    mtlxAddUint16(&b, ORION_TAG_PULL_PORT, (uint16_t)gs_pullPort);
    return mtlxPacketEnd(&b);
}

static uint16_t buildAliveMsg(uint8_t *buf, uint16_t sz, uint16_t transNum)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, ORION_FUNC_ALIVE);
    mtlxAddString(&b, ORION_TAG_DEVICE_DATE, dateStr);
    return mtlxPacketEnd(&b);
}

static uint16_t buildLogPacket(uint8_t *buf, uint16_t sz,
                                const char *logData, int packetNum,
                                BOOL stream, uint16_t transNum)
{
    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, ORION_FUNC_LOG);
    mtlxAddUint16(&b, ORION_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, ORION_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, ORION_TAG_LOG_DATA, logData);
    return mtlxPacketEnd(&b);
}

static uint16_t buildDataPacket(uint8_t *buf, uint16_t sz,
                                 uint8_t func, const char *meterId,
                                 const char *data, int packetNum,
                                 BOOL stream, uint16_t transNum)
{
    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, func);
    mtlxAddUint16(&b, ORION_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, ORION_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, ORION_TAG_METER_ID, meterId);
    mtlxAddString(&b, ORION_TAG_READOUT_DATA, data);
    return mtlxPacketEnd(&b);
}

static uint16_t buildDirectivePacket(uint8_t *buf, uint16_t sz,
                                      const char *directive,
                                      int packetNum, BOOL stream,
                                      uint16_t transNum)
{
    OrionBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, ORION_FUNC_DIRECTIVE_LIST);
    mtlxAddUint16(&b, ORION_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, ORION_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, ORION_TAG_DIRECTIVE_DATA, directive);
    return mtlxPacketEnd(&b);
}

/* ================================================================== */
/*               Packetised send helpers                              */
/* ================================================================== */

static void sendLogPackets(uint16_t transNum)
{
    char curBuf[ORION_DATA_CHUNK_SIZE + 1];
    char nxtBuf[ORION_DATA_CHUNK_SIZE + 1];

    S32 curLen = appLogRecRead(g_sysLoggerID, curBuf, ORION_DATA_CHUNK_SIZE);
    if (curLen <= 0)
    {
        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), "NO-LOG", 1, FALSE, transNum);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

        DEBUG_DEBUG("->[D] OrionTLV: no log packets to send (trans %u)", transNum);
        return;
    }

    int packetNum = 0;
    while (curLen > 0)
    {
        curBuf[curLen] = '\0';
        packetNum++;
        S32 nxtLen = appLogRecRead(g_sysLoggerID, nxtBuf, ORION_DATA_CHUNK_SIZE);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), curBuf, packetNum, hasMore, transNum);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

        if (!hasMore) break;
        memcpy(curBuf, nxtBuf, (size_t)nxtLen);
        curLen = nxtLen;
    }
}

static RETURN_STATUS sendFilePacketised(const char *channel, uint8_t func,
                                        const char *meterId, const char *filePath,
                                        uint16_t transNum)
{
    FsFile *f = fsOpenFile((char *)filePath, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    char bufA[ORION_DATA_CHUNK_SIZE + 1];
    char bufB[ORION_DATA_CHUNK_SIZE + 1];
    char *cur = bufA, *nxt = bufB;

    size_t curLen = 0;
    (void)fsReadFile(f, cur, ORION_DATA_CHUNK_SIZE, &curLen);
    if (curLen == 0)
    {
        fsCloseFile(f);
        return FAILURE;
    }

    int packetNum = 0;
    while (curLen > 0)
    {
        cur[curLen] = '\0';
        packetNum++;
        size_t nxtLen = 0;
        (void)fsReadFile(f, nxt, ORION_DATA_CHUNK_SIZE, &nxtLen);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildDataPacket(gs_txBuf, sizeof(gs_txBuf),
                                       func, meterId, cur, packetNum, hasMore, transNum);
        if (sz == 0)
        {
            fsCloseFile(f);
            return FAILURE;
        }
        if (SUCCESS != appTcpConnManagerSend(channel, (char *)gs_txBuf, (unsigned int)sz))
        {
            fsCloseFile(f);
            return FAILURE;
        }

        if (!hasMore) break;
        char *tmp = cur; cur = nxt; nxt = tmp;
        curLen = nxtLen;
    }
    fsCloseFile(f);
    return SUCCESS;
}

/* ================================================================== */
/*                      FTP URL parser                                */
/* ================================================================== */

static BOOL parseFtpUrl(const char *url,
                        char *host, size_t hostSz, U16 *port,
                        char *user, size_t userSz,
                        char *pass, size_t passSz,
                        char *path, size_t pathSz)
{
    if (strncmp(url, "ftp://", 6) != 0) return FALSE;
    const char *p = url + 6;

    const char *at = strchr(p, '@');
    if (at != NULL)
    {
        const char *colon = (const char *)memchr(p, ':', (size_t)(at - p));
        if (colon != NULL)
        {
            size_t uLen = (size_t)(colon - p); if (uLen >= userSz) uLen = userSz - 1;
            memcpy(user, p, uLen); user[uLen] = '\0';
            size_t pLen = (size_t)(at - colon - 1); if (pLen >= passSz) pLen = passSz - 1;
            memcpy(pass, colon + 1, pLen); pass[pLen] = '\0';
        }
        else
        {
            size_t uLen = (size_t)(at - p); if (uLen >= userSz) uLen = userSz - 1;
            memcpy(user, p, uLen); user[uLen] = '\0';
            pass[0] = '\0';
        }
        p = at + 1;
    }
    else
    {
        strncpy(user, "anonymous", userSz - 1); user[userSz - 1] = '\0';
        pass[0] = '\0';
    }

    const char *slash   = strchr(p, '/');
    const char *hostEnd = slash ? slash : (p + strlen(p));
    const char *portCol = (const char *)memchr(p, ':', (size_t)(hostEnd - p));

    if (portCol != NULL)
    {
        size_t hLen = (size_t)(portCol - p); if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen); host[hLen] = '\0';
        *port = (U16)atoi(portCol + 1);
    }
    else
    {
        size_t hLen = (size_t)(hostEnd - p); if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen); host[hLen] = '\0';
        *port = FTP_DEFAULT_PORT;
    }

    if (slash) { strncpy(path, slash, pathSz - 1); path[pathSz - 1] = '\0'; }
    else       { path[0] = '/'; path[1] = '\0'; }
    return TRUE;
}

/* ================================================================== */
/*                 FS helper  (read whole text file)                  */
/* ================================================================== */

static RETURN_STATUS readWholeText(const char *path, char *buf, size_t cap, size_t *outLen)
{
    FsFile *f = fsOpenFile((char *)path, FS_FILE_MODE_READ);
    if (f == NULL) return FAILURE;
    size_t total = 0;
    for (;;)
    {
        if (total + 1 >= cap) break;
        size_t chunk = 0;
        error_t e = fsReadFile(f, buf + total, cap - 1U - total, &chunk);
        if (e == ERROR_END_OF_FILE || chunk == 0) break;
        total += chunk;
    }
    buf[total] = '\0';
    if (outLen) *outLen = total;
    fsCloseFile(f);
    return SUCCESS;
}

/* ================================================================== */
/*                    Meter-task callback                             */
/* ================================================================== */

static void meterTaskCallback(S32 taskID, ERR_CODE_T status)
{
    for (int i = 0; i < MAX_PENDING_JOBS; i++)
    {
        if (taskID == gs_pendingJobs[i].taskId)
        {
            gs_pendingJobs[i].result    = status;
            gs_pendingJobs[i].completed = TRUE;
            break;
        }
    }
}

static int pendingJobAllocSlot(S32 taskId, BOOL isLoadProfile)
{
    for (int i = 0; i < MAX_PENDING_JOBS; i++)
    {
        if (gs_pendingJobs[i].taskId == 0)
        {
            gs_pendingJobs[i].taskId        = taskId;
            gs_pendingJobs[i].completed     = FALSE;
            gs_pendingJobs[i].result        = EN_ERR_CODE_PENDING;
            gs_pendingJobs[i].isLoadProfile = isLoadProfile;
            return i;
        }
    }
    return -1;
}

static void cleanupMeterFiles(S32 taskId)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char *)path);
    snprintf(path, sizeof(path), "%s%d_readout.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char *)path);
    snprintf(path, sizeof(path), "%s%d_payload.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char *)path);
}

/* ================================================================== */
/*           Session management                                       */
/* ================================================================== */

static void meterOprsOnComplete(OrionSession_t* session, OrionTLVState_t state, void *arg)
{    
    //if (SESSION_DONE == state)
    {
        if ((session->pendingJobIdx >= 0) && (session->pendingJobIdx < MAX_PENDING_JOBS))
        {
            PendingJob_t *job = &gs_pendingJobs[session->pendingJobIdx];
            
            cleanupMeterFiles(job->taskId);
            appMeterOperationsTaskIDFree(job->taskId);

            memset(job, 0, sizeof(*job));
            session->pendingJobIdx = -1;
        }
    }
}

static void completeSession(OrionSession_t* session, OrionTLVState_t state, void *arg)
{
    if (session->onComplateFunc)
        session->onComplateFunc(session, state, arg);
}

static void sessionDelete(OrionSession_t *s)
{
    if (s == NULL)
        return;    

    if (0 == strcmp(PUSH_TCP_SOCK_NAME, s->channel))
    {
        appTcpConnManagerRequestPushDisconnect();
        gsFlagList.tcpPushInUse = FALSE;
        DEBUG_INFO("->[I] sessionDelete push socket disconnected, Nightwish");
        zosDelayTask(1000);
    }

    memset(s, 0, sizeof(OrionSession_t));
}

static OrionSession_t *sessionCreate(uint8_t function, const char *channel,
                                     uint16_t transNum, OrionSessionCompleteCb onComplate)
{
    for (unsigned int i = 0; i < ORION_SESSION_MAX; i++)
    {
        if (gs_sessionTable[i].inUse == FALSE)
        {
            OrionSession_t *s = &gs_sessionTable[i];
            memset(s, 0, sizeof(*s));
            s->inUse            = TRUE;
            s->function         = function;
            s->transNumber      = transNum;
            s->step             = 0;            
            s->timeCnt          = 0;
            s->pendingJobIdx    = -1;
            s->rxReady          = FALSE;
            s->rxLen            = 0;
            s->onComplateFunc   = onComplate; 

            if (channel != NULL)
            {
                strncpy(s->channel, channel, sizeof(s->channel) - 1);
                s->channel[sizeof(s->channel) - 1] = '\0';
            }
            return s;
        }
    }
    return NULL;
}

static OrionSession_t *findSessionByTransNumber(uint16_t transNum)
{
    for (unsigned int i = 0; i < ORION_SESSION_MAX; i++)
    {
        if (gs_sessionTable[i].inUse && gs_sessionTable[i].transNumber == transNum)
            return &gs_sessionTable[i];
    }
    return NULL;
}

static BOOL sessionSetIncoming(OrionSession_t *s, const OrionIncomingMsg_t *msg)
{
    if (s == NULL || msg == NULL)
        return FALSE;
    if (msg->length == 0 || msg->length > ORION_PACKET_MAX_SIZE)
        return FALSE;

    memcpy(s->rxBuf, msg->payload, msg->length);
    s->rxLen = msg->length;
    s->rxReady = TRUE;

    if (msg->ch[0] != '\0')
    {
        strncpy(s->channel, msg->ch, sizeof(s->channel) - 1);
        s->channel[sizeof(s->channel) - 1] = '\0';
    }
    return TRUE;
}

static BOOL sessionGetParser(OrionSession_t *s, OrionParser_t *p)
{
    if (s == NULL || p == NULL)
        return FALSE;
    if (!s->rxReady || s->rxLen == 0)
        return FALSE;
    if (!mtlxParserInit(p, s->rxBuf, s->rxLen))
        return FALSE;
    return TRUE;
}

static BOOL mtlxReceiveEvent(OrionEventMsg_t *msg)
{
    if (msg == NULL || gs_evCount == 0U) return FALSE;
    *msg = gs_eventQ[gs_evHead];
    gs_evHead = (uint8_t)((gs_evHead + 1U) % ORION_EVENT_Q_DEPTH);
    gs_evCount--;
    return TRUE;
}

static void checkAllSessionTimeouts(void)
{
    for (unsigned int i = 0; i < ORION_SESSION_MAX; i++)
    {
        if (TRUE == gs_sessionTable[i].inUse)
        {
            OrionSession_t *s = &gs_sessionTable[i];
            s->timeCnt += 1000U; //1 second

            if (s->timeCnt > ORION_SESSION_TIMEOUT)
            {
                DEBUG_WARNING("->[W] OrionTLV: session timeout (func 0x%02X, trans %u)",
                            s->function, s->transNumber);
                
                completeSession(s, SESSION_TIMEOUT, NULL);
                sessionDelete(s);
            }
        }
    }
}

static void periodicTimerCb(void)
{
    checkAllSessionTimeouts();

    gs_aliveCnt++;
    if (gs_aliveCnt >= (uint32_t)ORION_ALIVE_INTERVAL_S)
    {
        gsFlagList.sendAlive = TRUE;
        gs_aliveCnt  = 0;
    }
}

/**
 * Start a push session to send readout/loadprofile file chunks to the server.
 * Owns gs_pendingJobs[jobIdx] until send + ACK wait completes (or timeout).
 */
static RETURN_STATUS startPushMeterDataSession(int jobIdx, uint16_t transNum)
{
    if (jobIdx < 0 || jobIdx >= MAX_PENDING_JOBS)
        return FAILURE;

    OrionSession_t *s = sessionCreate(SFUNC_PUSH_METER_DATA, PUSH_TCP_SOCK_NAME, transNum, meterOprsOnComplete);
    if (NULL == s)
        return FAILURE;

    s->pendingJobIdx = jobIdx;
    s->step          = 0;
    s->timeCnt       = 0;
    
    return SUCCESS;
}

/* ================================================================== */
/*          Session handlers — step-based state machines              */
/* ================================================================== */
/**
 * Push meter data over TCP — non-blocking steps (like processAliveSession).
 * step 0: wait for push socket
 * step 1: send packetised file
 * step 2: wait for server ACK on push (same transactionNumber routes via dispatch)
 */
static void processPushMeterDataSession(OrionSession_t *session)
{
    int jobIdx = session->pendingJobIdx;
    if (jobIdx < 0 || jobIdx >= MAX_PENDING_JOBS)
    {
        sessionDelete(session);
        return;
    }

    PendingJob_t *job = &gs_pendingJobs[jobIdx];
    S32 taskId = job->taskId;
    if (taskId == 0)
    {
        sessionDelete(session);
        return;
    }

    uint8_t func = job->isLoadProfile ? ORION_FUNC_LOADPROFILE : ORION_FUNC_READOUT;

    char meterId[128] = {0};
    char idPath[MAX_PATH_LEN];
    snprintf(idPath, sizeof(idPath), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)taskId);

    size_t got = 0;
    if (SUCCESS != readWholeText(idPath, meterId, sizeof(meterId), &got) || got == 0)
        strncpy(meterId, "unknown", sizeof(meterId));

    if (job->isLoadProfile)
        strncpy(meterId, "load-profile-no-id", sizeof(meterId));

    char dataPath[MAX_PATH_LEN];
    if (job->isLoadProfile)
        snprintf(dataPath, sizeof(dataPath), "%s%d_payload.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    else
        snprintf(dataPath, sizeof(dataPath), "%s%d_readout.txt", METER_DATA_OUTPUT_DIR, (int)taskId);

    switch (session->step)
    {
        case 0:
        {
            if (CONN_TYPE_TCP == gsFlagList.connType)
            {
                if ((FALSE == gsFlagList.tcpPushInUse) && (FALSE == appTcpConnManagerIsConnectedPush()))
                {
                    gsFlagList.tcpPushInUse = TRUE;
                    appTcpConnManagerRequestPushConnect();

                    session->timeCnt = 0; //clear next step timeout counter
                    session->step    = 1;

                    DEBUG_INFO("->[I] OrionTLV: Push Socket is used in PushIdent Session");                
                }

                zosDelayTask(1000);
                break;
            }            
        }
        case 1:
        {
            if ((CONN_TYPE_TCP == gsFlagList.connType) && (FALSE == appTcpConnManagerIsConnectedPush()))
            {
                appTcpConnManagerRequestPushConnect();
                break;
            }

            if (SUCCESS != sendFilePacketised(PUSH_TCP_SOCK_NAME, func, meterId, dataPath, session->transNumber))
            {
                DEBUG_ERROR("->[E] OrionTLV: sendFilePacketised failed (trans %u) for meter task %d", session->transNumber, taskId);

                completeSession(session, SESSION_ERROR, NULL);
                sessionDelete(session);
                break;
            }
            session->rxReady = FALSE;
            session->rxLen   = 0;
            session->timeCnt = 0; //clear next step timeout counter
            session->step    = 2;
            break;
        }
        case 2:
        {
            if (session->rxReady)
            {
                session->rxReady = FALSE;
                session->rxLen   = 0;
                
                completeSession(session, SESSION_DONE, NULL);
                sessionDelete(session);
            }
            /* timeout will be handled by the session timeout handler 
            * we dont need to call sessionDelete here, it will be called
            * by the session timeout handler */

            break;
        }
    }
}

/* ── Push Ident (registration) ── */
static void processIdentSession(OrionSession_t *session)
{
    switch (session->step)
    {
        case 0:
        {
            if (CONN_TYPE_TCP == gsFlagList.connType)
            {
                if ((FALSE == gsFlagList.tcpPushInUse) && (FALSE == appTcpConnManagerIsConnectedPush()))
                {
                    gsFlagList.tcpPushInUse = TRUE;
                    appTcpConnManagerRequestPushConnect();

                    session->timeCnt = 0;
                    session->step    = 1;

                    DEBUG_INFO("->[I] OrionTLV: Push Socket is used in PushIdent Session");                
                }

                zosDelayTask(1000);
                break;
            }            
        }
        case 1:
        {
            if ((CONN_TYPE_TCP == gsFlagList.connType) && (FALSE == appTcpConnManagerIsConnectedPush()))
            {
                appTcpConnManagerRequestPushConnect();
                zosDelayTask(1000);
                break;
            }

            uint16_t sz = buildIdentMsg(gs_txBuf, sizeof(gs_txBuf), session->transNumber);
            if (SUCCESS != appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz))
            {
                DEBUG_WARNING("->[W] OrionTLV: pushident send failed");
                APP_LOG_REC(g_sysLoggerID, "Pushident send failed");
                gsFlagList.sendIdent = TRUE;
                
                sessionDelete(session);
                break;
            }

            DEBUG_INFO("->[I] OrionTLV: pushident sent tarnsNum: %d", session->transNumber);

            session->timeCnt = 0;
            session->step    = 2;
            break;
        }

        case 2:
        {
            if (session->rxReady)
            {
                OrionParser_t rp;
                if (sessionGetParser(session, &rp))
                {
                    uint16_t rxTrans = 0;
                    mtlxGetUint16(&rp, ORION_TAG_TRANS_NUMBER, &rxTrans);

                    if (rxTrans == session->transNumber)
                    {
                        uint8_t rFunc = 0;
                        if (mtlxGetUint8(&rp, ORION_TAG_FUNCTION, &rFunc)
                            && rFunc == ORION_FUNC_IDENT)
                        {
                            BOOL regVal = FALSE;
                            if (mtlxGetBool(&rp, ORION_TAG_REGISTER, &regVal) && regVal)
                            {
                                gs_registered = TRUE;
                                registerStateSave(TRUE);
                                DEBUG_INFO("->[I] OrionTLV: device registered");
                                APP_LOG_REC(g_sysLoggerID, "Device registered");
                            }
                        }
                    }
                }

                gsFlagList.sendIdent = !gs_registered;

                session->rxReady = FALSE;
                session->rxLen = 0;
                sessionDelete(session);
            }

            break;
        }            
    }
}

/* ── Alive ── */
static void processAliveSession(OrionSession_t *session)
{
    switch (session->step)
    {
        case 0:
        {
            if (CONN_TYPE_TCP == gsFlagList.connType)
            {
                if ((FALSE == gsFlagList.tcpPushInUse) && (FALSE == appTcpConnManagerIsConnectedPush()))
                {
                    gsFlagList.tcpPushInUse = TRUE;
                    appTcpConnManagerRequestPushConnect();

                    session->timeCnt = 0;
                    session->step    = 1;

                    DEBUG_INFO("->[I] OrionTLV: Push Socket is used in Alive Session");                
                }

                zosDelayTask(1000);
                break;
            }            
        }
        case 1:
        {
            if ((CONN_TYPE_TCP == gsFlagList.connType) && (FALSE == appTcpConnManagerIsConnectedPush()))
            {
                appTcpConnManagerRequestPushConnect();
                break;
            }

            uint16_t sz = buildAliveMsg(gs_txBuf, sizeof(gs_txBuf), session->transNumber);

            if(SUCCESS != appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz))
            {
                DEBUG_WARNING("->[W] OrionTLV: alive send failed (trans %u)", session->transNumber);
                APP_LOG_REC(g_sysLoggerID, "Alive send failed");
                
                session->timeCnt = 0;
                break;
            }
            else 
            {
                DEBUG_DEBUG("->[D] OrionTLV: alive sent (trans %u)", session->transNumber);
            }       

            sessionDelete(session);
            break;
        }
    }    
}

/* ── Pull-socket request handlers ── */
static void processLogSession(OrionSession_t *session)
{
    DEBUG_INFO("->[I] OrionTLV: log request (trans %u)", session->transNumber);

    sendLogPackets(session->transNumber);
    sessionDelete(session);
}

static void processSettingSession(OrionSession_t *session)
{
    DEBUG_INFO("->[I] OrionTLV: setting request (trans %u)", session->transNumber);
    OrionParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }
    BOOL success = TRUE;

    char newIP[20] = {0};
    uint16_t newPort = 0;
    BOOL hasIP   = mtlxGetString(&parser, ORION_TAG_SERVER_IP, newIP, sizeof(newIP));
    BOOL hasPort = mtlxGetUint16(&parser, ORION_TAG_SERVER_PORT, &newPort);

    if (hasIP && newIP[0] != '\0' && hasPort && newPort > 0)
    {
        strncpy(gs_serverIP, newIP, sizeof(gs_serverIP) - 1);
        gs_serverPort = (int)newPort;
        connConfigSave();

        appTcpConnManagerStop();
        zosDelayTask(500);
        appTcpConnManagerStart(gs_serverIP, gs_serverPort,
                               gs_pullPort, appProtocolOrionTLVPutIncomingMessage);
        appTcpConnManagerRequestConnect();

        DEBUG_INFO("->[I] OrionTLV: server updated to %s:%d", gs_serverIP, gs_serverPort);
        APP_LOG_REC(g_sysLoggerID, "Server address updated");
    }

    orionParserReset(&parser);

    uint16_t tag; const uint8_t *val; uint16_t vLen;
    BOOL inMeter = FALSE;
    char operation[16] = {0};
    MeterData_t md;

    while (orionParserNext(&parser, &tag, &val, &vLen))
    {
        if (tag == ORION_TAG_METER_INDEX)
        {
            if (inMeter)
            {
                if (0 == strcmp(operation, "add"))
                { 
                    if (SUCCESS != appMeterOperationsAddMeter(&md)) success = FALSE; 
                }
                else if (0 == strcmp(operation, "remove"))
                { 
                    if (md.serialNumber[0] != '\0') 
                    {
                        if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber)) success = FALSE; 
                    }
                }
            }

            inMeter = TRUE;
            memset(&md, 0, sizeof(md));
            memset(operation, 0, sizeof(operation));
            continue;
        }
        if (!inMeter) continue;

        switch (tag)
        {
            case ORION_TAG_METER_OPERATION:
            { 
                uint16_t cp = vLen < sizeof(operation)-1 ? vLen : (uint16_t)(sizeof(operation)-1);
                memcpy(operation, val, cp); operation[cp]='\0'; 
                break; 
            }
            case ORION_TAG_METER_PROTOCOL:
            { 
                uint16_t cp = vLen < sizeof(md.protocol)-1 ? vLen : (uint16_t)(sizeof(md.protocol)-1); 
                memcpy(md.protocol, val, cp); md.protocol[cp]='\0';
                break;
            }
            case ORION_TAG_METER_TYPE:
            { 
                uint16_t cp = vLen < sizeof(md.type)-1 ? vLen : (uint16_t)(sizeof(md.type)-1);
                memcpy(md.type, val, cp); md.type[cp]='\0';
                break;
            }
            case ORION_TAG_METER_BRAND:
            { 
                uint16_t cp = vLen < sizeof(md.brand)-1 ? vLen : (uint16_t)(sizeof(md.brand)-1);
                memcpy(md.brand, val, cp); md.brand[cp]='\0';
                break;
            }
            case ORION_TAG_METER_SERIAL_NUM:
            { 
                uint16_t cp = vLen < sizeof(md.serialNumber)-1 ? vLen : (uint16_t)(sizeof(md.serialNumber)-1);
                memcpy(md.serialNumber, val, cp); md.serialNumber[cp]='\0';
                break;
            }
            case ORION_TAG_METER_SERIAL_PORT:
            { 
                uint16_t cp = vLen < sizeof(md.serialPort)-1 ? vLen : (uint16_t)(sizeof(md.serialPort)-1); 
                memcpy(md.serialPort, val, cp); md.serialPort[cp]='\0';
                break;
            }
            case ORION_TAG_METER_INIT_BAUD:
            { 
                if (vLen >= 4) md.initBaud = (int)(((uint32_t)val[0]<<24)|((uint32_t)val[1]<<16)|((uint32_t)val[2]<<8)|val[3]);
                break;
            }
            case ORION_TAG_METER_FIX_BAUD:
            {
                md.fixBaud = (vLen >= 1 && val[0] != 0) ? TRUE : FALSE;
                break;
            }
            case ORION_TAG_METER_FRAME:
            { 
                uint16_t cp = vLen < sizeof(md.frame)-1 ? vLen : (uint16_t)(sizeof(md.frame)-1);
                memcpy(md.frame, val, cp); md.frame[cp]='\0';
                break;
            }
            case ORION_TAG_METER_CUSTOMER_NUM:
            { 
                uint16_t cp = vLen < sizeof(md.customerNumber)-1 ? vLen : (uint16_t)(sizeof(md.customerNumber)-1);
                memcpy(md.customerNumber, val, cp); md.customerNumber[cp]='\0';
                break;
            }
            default: break;
        }
    }

    if (inMeter)
    {
        if (0 == strcmp(operation, "add"))
        { 
            if (SUCCESS != appMeterOperationsAddMeter(&md)) success = FALSE; 
        }
        else if (0 == strcmp(operation, "remove"))
        { 
            if (md.serialNumber[0] != '\0') if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber)) success = FALSE; 
        }
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), success, session->transNumber);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

static void processFwUpdateSession(OrionSession_t *session)
{
    DEBUG_INFO("->[I] OrionTLV: fwUpdate request (trans %u)", session->transNumber);
    OrionParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char address[192] = {0};
    if (!mtlxGetString(&parser, ORION_TAG_FW_ADDRESS, address, sizeof(address)) || address[0] == '\0')
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    char host[FTP_FIELD_MAX], user[FTP_FIELD_MAX], pass[FTP_FIELD_MAX], path[FTP_FIELD_MAX];
    U16 port = FTP_DEFAULT_PORT;
    if (parseFtpUrl(address, host, sizeof(host), &port,
                    user, sizeof(user), pass, sizeof(pass),
                    path, sizeof(path)))
    {
        //AppSwUpdateInit(host, port, user, pass, path, FW_LOCAL_PATH);
        DEBUG_INFO("->[I] OrionTLV: FW update started from %s", host);
        APP_LOG_REC(g_sysLoggerID, "FW update started");
    }

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

/* ── Readout — multi-step ── */
static void processReadoutSession(OrionSession_t *session)
{
    OrionParser_t parser;

    switch (session->step)
    {
        case 0:
        {
            uint16_t sz;

            DEBUG_INFO("->[I] OrionTLV: readout request (trans %u)", session->transNumber);
            if (!sessionGetParser(session, &parser))
            {
                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session);
                return;
            }
            char directive[64] = {0}, meterSn[20] = {0};
            mtlxGetString(&parser, ORION_TAG_DIRECTIVE_NAME, directive, sizeof(directive));
            mtlxGetString(&parser, ORION_TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));

            char reqBuf[320];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\"}}",
                     directive, meterSn);

            S32 tid = appMeterOperationsAddReadoutTask(reqBuf, meterTaskCallback);
            if (tid < 0)
            {
                DEBUG_ERROR("->[E] OrionTLV: meter oprs READOUT task add failed tranN %u", session->transNumber);
                APP_LOG_REC(g_sysLoggerID, "OrionTLV: meter oprs READOUT task add failed");

                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); 
                return;
            }

            int slot = pendingJobAllocSlot(tid, FALSE);
            if (slot < 0)
            {
                appMeterOperationsTaskIDFree(tid);
                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session);
                return;
            }

            session->pendingJobIdx = slot;            

            sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            
            session->step = 1;
            session->timeCnt = 0; //clear next step timeout counter
            
            session->rxReady = FALSE;
            session->rxLen = 0;
            break;
        }

        case 1:
            if ((session->pendingJobIdx >= 0) && (gs_pendingJobs[session->pendingJobIdx].completed))
            {
                int idx = session->pendingJobIdx;
                S32 tid = gs_pendingJobs[idx].taskId;
                ERR_CODE_T res = gs_pendingJobs[idx].result;

                if (res == EN_ERR_CODE_SUCCESS)
                {
                    if (FAILURE == startPushMeterDataSession(idx, session->transNumber))
                    {
                        DEBUG_ERROR("->[E] OrionTLV: push meter data session start failed tranN %u", session->transNumber);
                        APP_LOG_REC(g_sysLoggerID, "Push meter data session start failed");
                        cleanupMeterFiles(tid);
                        appMeterOperationsTaskIDFree(tid);
                        memset(&gs_pendingJobs[idx], 0, sizeof(gs_pendingJobs[idx]));
                    }
                }
                else
                {
                    DEBUG_WARNING("->[W] OrionTLV: readout task failed tranN %u, tid %d, err %d",
                                  session->transNumber, (int)tid, (int)res);

                    APP_LOG_REC(g_sysLoggerID, "Readout task failed");

                    cleanupMeterFiles(tid);
                    appMeterOperationsTaskIDFree(tid);
                    memset(&gs_pendingJobs[idx], 0, sizeof(gs_pendingJobs[idx]));
                }

                sessionDelete(session);
            }
            break;

        default:
            sessionDelete(session);
            break;
    }
}

/* ── Load Profile — multi-step ── */
static void processLoadProfileSession(OrionSession_t *session)
{
    OrionParser_t parser;

    switch (session->step)
    {
        case 0:
        {
            uint16_t sz;

            DEBUG_INFO("->[I] OrionTLV: loadprofile request (trans %u)", session->transNumber);
            if (!sessionGetParser(session, &parser))
            {
                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session);
                return;
            }
            char directive[64] = {0}, meterSn[20] = {0};
            char startDate[24] = {0}, endDate[24] = {0};

            mtlxGetString(&parser, ORION_TAG_DIRECTIVE_NAME, directive, sizeof(directive));
            mtlxGetString(&parser, ORION_TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));
            mtlxGetString(&parser, ORION_TAG_START_DATE, startDate, sizeof(startDate));
            mtlxGetString(&parser, ORION_TAG_END_DATE, endDate, sizeof(endDate));

            char reqBuf[384];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\""
                     ",\"startDate\":\"%s\",\"endDate\":\"%s\"}}",
                     directive, meterSn, startDate, endDate);

            S32 tid = appMeterOperationsAddProfileTask(reqBuf, meterTaskCallback);
            if (tid <= 0)
            {
                DEBUG_ERROR("->[E] OrionTLV: meter oprs PROFILE task add failed tranN %u", session->transNumber);
                APP_LOG_REC(g_sysLoggerID, "OrionTLV: meter oprs PROFILE task add failed tranN");

                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            int slot = pendingJobAllocSlot(tid, TRUE);
            if (slot < 0)
            {
                DEBUG_ERROR("->[E] OrionTLV: meter oprs LP task slot allocation failed tranN %u", session->transNumber);
                APP_LOG_REC(g_sysLoggerID, "OrionTLV: meter oprs LP task slot allocation failed tranN");

                appMeterOperationsTaskIDFree(tid);
                sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            session->pendingJobIdx = slot;

            sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            
            session->step = 1;
            session->timeCnt = 0; //clear next step timeout counter

            session->rxReady = FALSE;
            session->rxLen = 0;
            break;
        }
        case 1:
        {
            if (TRUE == gs_pendingJobs[session->pendingJobIdx].completed) //check if the task is completed
            {
                int idx = session->pendingJobIdx;
                S32 tid = gs_pendingJobs[idx].taskId;
                ERR_CODE_T res = gs_pendingJobs[idx].result;

                if (res == EN_ERR_CODE_SUCCESS)
                {
                    if (FAILURE == startPushMeterDataSession(idx, session->transNumber))
                    {
                        DEBUG_ERROR("->[E] OrionTLV: LP, push meter data session start failed tranN %u",
                                    session->transNumber);
                        APP_LOG_REC(g_sysLoggerID, "OrionTLV: LP, push meter data session start failed tranN");
                        cleanupMeterFiles(tid);
                        appMeterOperationsTaskIDFree(tid);
                        memset(&gs_pendingJobs[idx], 0, sizeof(gs_pendingJobs[idx]));
                    }
                }
                else
                {
                    DEBUG_WARNING("->[W] OrionTLV: LP task failed (err %d)", (int)res);
                    cleanupMeterFiles(tid);
                    appMeterOperationsTaskIDFree(tid);
                    memset(&gs_pendingJobs[idx], 0, sizeof(gs_pendingJobs[idx]));
                }

                sessionDelete(session);
            }
            break;
        }
    }
}

static void processDirectiveListSession(OrionSession_t *session)
{
    OrionParser_t parser;
    uint16_t sz;
    
    DEBUG_INFO("->[I] OrionTLV: directiveList request (trans %u)", session->transNumber);
    
    if (!sessionGetParser(session, &parser))
    {
        sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char filterId[64] = {0};
    mtlxGetString(&parser, ORION_TAG_DIRECTIVE_ID, filterId, sizeof(filterId));

    BOOL sendAll = (filterId[0] == '\0' || (filterId[0] == '*' && filterId[1] == '\0'));

    U32 count = appMeterOperationsGetDirectiveCount();
    if (count == 0)
    {
        sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    char dirBuf[ORION_PACKET_MAX_SIZE];
    int totalToSend = 0;
    if (sendAll) totalToSend = (int)count;
    else { if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf))) totalToSend = 1; }

    if (totalToSend == 0)
    {
        sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    int sent = 0, packetNum = 0;
    if (sendAll)
    {
        for (U32 i = 0; i < count; i++)
        {
            if (SUCCESS != appMeterOperationsGetDirectiveByIndex(i, dirBuf, sizeof(dirBuf)))
                continue;
            sent++; packetNum++;
            sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf,
                                               packetNum, (sent < totalToSend), session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
        {
            sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf,
                                               1, FALSE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

static void processDirectiveAddSession(OrionSession_t *session)
{
    DEBUG_INFO("->[I] OrionTLV: directiveAdd request (trans %u)", session->transNumber);
    OrionParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char dirObj[ORION_PACKET_MAX_SIZE];
    if (!mtlxGetString(&parser, ORION_TAG_DIRECTIVE_DATA, dirObj, sizeof(dirObj)))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    S32 result = appMeterOperationsAddDirective(dirObj);
    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), (result >= 0), session->transNumber);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

static void processDirectiveDeleteSession(OrionSession_t *session)
{
    DEBUG_INFO("->[I] OrionTLV: directiveDelete request (trans %u)", session->transNumber);
    OrionParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char filterId[64] = {0};
    mtlxGetString(&parser, ORION_TAG_DIRECTIVE_ID, filterId, sizeof(filterId));

    if (filterId[0] == '\0')
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    BOOL ok = (SUCCESS == appMeterOperationsDeleteDirective(filterId));
    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), ok, session->transNumber);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

/* ================================================================== */
/*          Session dispatcher                                        */
/* ================================================================== */

static void processSessionMessage(OrionSession_t *session)
{
    switch (session->function)
    {
        case SFUNC_PUSH_IDENT:          processIdentSession(session);           break;
        case SFUNC_ALIVE:               processAliveSession(session);           break;
        case SFUNC_PUSH_METER_DATA:     processPushMeterDataSession(session);   break;
        case ORION_FUNC_LOG:            processLogSession(session);             break;
        case ORION_FUNC_SETTING:        processSettingSession(session);         break;
        case ORION_FUNC_FW_UPDATE:      processFwUpdateSession(session);        break;
        case ORION_FUNC_READOUT:        processReadoutSession(session);         break;
        case ORION_FUNC_LOADPROFILE:    processLoadProfileSession(session);     break;
        case ORION_FUNC_DIRECTIVE_LIST: processDirectiveListSession(session);   break;
        case ORION_FUNC_DIRECTIVE_ADD:  processDirectiveAddSession(session);    break;
        case ORION_FUNC_DIRECTIVE_DEL:  processDirectiveDeleteSession(session); break;
        default:
            DEBUG_WARNING("->[W] OrionTLV: unknown func 0x%02X", session->function);
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(session->channel, (char *)gs_txBuf, (unsigned int)sz);
            }
            sessionDelete(session);
            break;
    }
}

/* ================================================================== */
/*          Incoming message handlers                                 */
/* ================================================================== */

static void dispatchIncomingMessage(const OrionIncomingMsg_t *msg)
{
    if (msg == NULL || msg->length == 0)
        return;

    OrionParser_t parser;
    if (!mtlxParserInit(&parser, msg->payload, msg->length))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, 0);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    uint8_t func = 0;
    uint16_t transNum = 0;

    if (!mtlxGetUint8(&parser, ORION_TAG_FUNCTION, &func))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, 0);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }
    mtlxGetUint16(&parser, ORION_TAG_TRANS_NUMBER, &transNum);

    /* if transNum belongs to an active session, route it */
    OrionSession_t *session = (transNum != 0) ? findSessionByTransNumber(transNum) : NULL;
    if (session)
    {
        (void)sessionSetIncoming(session, msg);
        processSessionMessage(session);
        return;
    }

    /* start new session */
    session = sessionCreate(func, msg->ch, transNum, NULL);
    if (session)
    {
        (void)sessionSetIncoming(session, msg);
        processSessionMessage(session);
    }
    else
    {
        DEBUG_WARNING("->[W] OrionTLV: too many sessions");
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, transNum);
        appTcpConnManagerSend(msg->ch, (char *)gs_txBuf, (unsigned int)sz);
    }
}

static void handleEvent(OrionEventMsg_t *evt)
{
    switch (evt->type)
    {
        case ORION_EVENT_FW_UPGRADE_SUCCESS:
            gsFlagList.sendIdent = TRUE;
            break;

        case ORION_EVENT_ALIVE:
            gsFlagList.sendAlive = TRUE;
            break;

        default:
            break;
    }
}

/* ================================================================== */
/*                     Protocol task                                  */
/* ================================================================== */

static void protocolTaskFunc(void *arg)
{
    (void)arg;
    DEBUG_INFO("->[I] %s task started", ORION_TASK_NAME);
    appTskMngImOK(gs_taskId);

    if (0 != appTcpConnManagerStart(gs_serverIP, gs_serverPort,
                                     gs_pullPort, appProtocolOrionTLVPutIncomingMessage))
    {
        DEBUG_ERROR("->[E] OrionTLV: TCP start failed");
        APP_LOG_REC(g_sysLoggerID, "OrionTLV: TCP start fail");
        return;
    }
    appTcpConnManagerRequestConnect();
    zosDelayTask(500);

    gs_registered = registerStateLoad();

    if (!gs_registered)
    {
        DEBUG_INFO("->[I] OrionTLV: not registered, starting ident session");
        gsFlagList.sendIdent = TRUE;
    }
    else
    {
        DEBUG_INFO("->[I] OrionTLV: already registered");
    }

    while (666)
    {
        appTskMngImOK(gs_taskId);

        /* 1. Incoming messages (push + pull) */
        {
            OrionIncomingMsg_t in;
            while (inQueuePop(&in))
                dispatchIncomingMessage(&in);
        }

        /* 2. Events */
        {
            OrionEventMsg_t evMsg;
            while (mtlxReceiveEvent(&evMsg))
                handleEvent(&evMsg);
        }

        /* 3. Process all active sessions */
        for (unsigned int i = 0; i < ORION_SESSION_MAX; i++)
        {
            if (gs_sessionTable[i].inUse)
                processSessionMessage(&gs_sessionTable[i]);
        }

        /* 4. Task flags */
        if (gsFlagList.sendIdent)
        {
            if (sessionCreate(SFUNC_PUSH_IDENT, PUSH_TCP_SOCK_NAME, nextTransNumber(), NULL) != NULL)
                gsFlagList.sendIdent = FALSE;
        }

        if (gsFlagList.sendAlive && gs_registered)
        {
            if (sessionCreate(SFUNC_ALIVE, PUSH_TCP_SOCK_NAME, nextTransNumber(), NULL) != NULL)
                gsFlagList.sendAlive = FALSE;
        }

        zosDelayTask(100);
    }

    appTcpConnManagerStop();
    DEBUG_INFO("->[I] %s task stopped", ORION_TASK_NAME);
}

/* ================================================================== */
/*                       PUBLIC FUNCTIONS                             */
/* ================================================================== */

RETURN_STATUS appProtocolOrionTLVInit(const char *serialNumber)
{
    if (NULL == serialNumber)
        return FAILURE;

    memset(gs_serialNumber, 0, sizeof(gs_serialNumber));
    strncpy(gs_serialNumber, serialNumber, sizeof(gs_serialNumber) - 1);

    if (FAILURE == connConfigLoad())
    {
        connConfigDefault();
        connConfigSave();
    }

    memset(gs_pendingJobs, 0, sizeof(gs_pendingJobs));
    memset(gs_sessionTable, 0, sizeof(gs_sessionTable));

    gs_inHead = gs_inTail = gs_inCount = 0;

    gs_evHead = gs_evTail = gs_evCount = 0;

    gs_registered = registerStateLoad();
    gs_taskId     = OS_INVALID_TASK_ID;
    gs_periodicTimerId = -1;
    gsFlagList.sendIdent  = FALSE;
    gsFlagList.sendAlive  = FALSE;
    gs_aliveCnt   = 0;
    gs_localTransNum = 0;

    gsFlagList.connType = CONN_TYPE_TCP;

    return SUCCESS;
}

RETURN_STATUS appProtocolOrionTLVStart(void)
{
    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = ORION_TASK_STACK_SIZE;

    gs_taskId = appTskMngCreate(ORION_TASK_NAME, protocolTaskFunc, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_taskId)
    {
        DEBUG_ERROR("->[E] OrionTLV: task creation failed");
        APP_LOG_REC(g_sysLoggerID, "OrionTLV task creation failed");
        return FAILURE;
    }

    if (gs_periodicTimerId < 0)
    {
        if (SUCCESS == middEventTimerRegister(&gs_periodicTimerId, periodicTimerCb, TIME_OUT_1_SEC, TRUE))
        {
            DEBUG_WARNING("->[W] OrionTLV: periodic timer registered successfully");
            (void)middEventTimerStart(gs_periodicTimerId); //not needed to check the return value
        }
    }
  
    DEBUG_INFO("->[I] OrionTLV: started");
    APP_LOG_REC(g_sysLoggerID, "Protocol OrionTLV started");
  
    return SUCCESS;
}

RETURN_STATUS appProtocolOrionTLVStop(void)
{
    if (gs_periodicTimerId >= 0)
    {
        (void)middEventTimerStop(gs_periodicTimerId);
        (void)middEventTimerDelete(gs_periodicTimerId);
        gs_periodicTimerId = -1;
    }

    zosDelayTask(500);

    if (OS_INVALID_TASK_ID != gs_taskId)
    {
        appTskMngDelete(&gs_taskId);
        gs_taskId = OS_INVALID_TASK_ID;
    }

    DEBUG_INFO("->[I] OrionTLV: stopped");
    APP_LOG_REC(g_sysLoggerID, "Protocol OrionTLV stopped");
    return SUCCESS;
}

void appProtocolOrionTLVPutIncomingMessage(const char *channel,
                                          const char *data,
                                          unsigned int dataLength)
{
    if (channel == NULL || data == NULL || dataLength == 0)
        return;

    (void)inQueuePush(channel, (const uint8_t *)data, (uint16_t)dataLength);
}

BOOL appProtocolOrionTLVSendEvent(OrionEventType_t type)
{
    if (gs_evCount >= ORION_EVENT_Q_DEPTH)
        return FALSE;
    gs_eventQ[gs_evTail].type = type;
    gs_evTail = (uint8_t)((gs_evTail + 1U) % ORION_EVENT_Q_DEPTH);
    gs_evCount++;
    return TRUE;
}

/********************************* End Of File ********************************/
