/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 30, 2026
* #File Name    : AppProtocolMetalix.c
*******************************************************************************/
/******************************************************************************
* Metalix TLV-over-TCP protocol service with session management.
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
#include "AppProtocolMetalix.h"
#include "MiddEventTimer.h"
#include "AppTcpConnManager.h"
#include "AppMeterOperations.h"
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
#define MTLX_TASK_STACK_SIZE    (4096U)
#define MTLX_TASK_NAME          "MtlxSess"

#define MAX_PENDING_JOBS        (4)
#define MAX_PATH_LEN            (96)

#define FTP_FIELD_MAX           (64)
#define FTP_DEFAULT_PORT        (21)
#define FW_LOCAL_PATH           "/tmp/firmware.bin"

#define TLV_HEADER_SIZE         (4)   /* 2 tag + 2 len */

#define MTLX_EVENT_Q_DEPTH     (8)

#define SFUNC_PUSH_IDENT        (0xF0)
#define SFUNC_ALIVE             (0xF1)

/******************************* TYPE DEFINITIONS *****************************/

typedef struct
{
    char     ch[16];
    uint8_t  payload[MTLX_PACKET_MAX_SIZE];
    uint16_t length;
} MtlxIncomingMsg_t;

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
} ServerConfig_t;

typedef enum
{
    MTLX_SESS_DONE = 0,
    MTLX_SESS_ERROR,
    MTLX_SESS_TIMEOUT,
} MtlxSessState_t;

typedef struct
{
    BOOL        inUse;
    uint8_t     function;
    char        channel[16];
    uint16_t    transNumber;

    uint32_t    step;
    uint32_t    timeoutGobackStep;
    uint32_t    timeCnt;
    uint32_t    retryIntervalMs;
    uint32_t    retryCount;

    uint8_t     rxBuf[MTLX_PACKET_MAX_SIZE];
    uint16_t    rxLen;
    volatile BOOL rxReady;

    int         pendingJobIdx;
} MtlxSession_t;

typedef struct
{
    MtlxEventType_t type;
} MtlxEventMsg_t;

/********************************** VARIABLES *********************************/

static char gs_serialNumber[20];
static char gs_serverIP[20];
static int  gs_serverPort;
static char gs_deviceIP[20];
static int  gs_pullPort;

static BOOL gs_registered = FALSE;
static volatile BOOL gs_running = FALSE;
static OsTaskId gs_taskId = OS_INVALID_TASK_ID;

static PendingJob_t gs_pendingJobs[MAX_PENDING_JOBS];

static uint8_t gs_txBuf[MTLX_PACKET_MAX_SIZE];

static MtlxSession_t gs_sessionTable[MTLX_SESSION_MAX];

/* Incoming message ring buffer (replaces single gs_pushRx/gs_pullRx buffers) */
#define MTLX_INMSG_Q_DEPTH   (8)
static MtlxIncomingMsg_t gs_inQ[MTLX_INMSG_Q_DEPTH];
static volatile uint8_t gs_inHead;
static volatile uint8_t gs_inTail;
static volatile uint8_t gs_inCount;

static MtlxEventMsg_t gs_eventQ[MTLX_EVENT_Q_DEPTH];
static uint8_t gs_evHead;
static uint8_t gs_evTail;
static uint8_t gs_evCount;

static S32 gs_periodicTimerId = -1;

static volatile BOOL gs_sendIdent  = FALSE;
static volatile BOOL gs_sendAlive  = FALSE;
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

    if (len > MTLX_PACKET_MAX_SIZE)
        len = MTLX_PACKET_MAX_SIZE;

    if (gs_inCount >= MTLX_INMSG_Q_DEPTH)
        return FALSE;

    MtlxIncomingMsg_t *it = &gs_inQ[gs_inTail];
    memset(it, 0, sizeof(*it));
    if (ch != NULL)
    {
        strncpy(it->ch, ch, sizeof(it->ch) - 1);
        it->ch[sizeof(it->ch) - 1] = '\0';
    }
    memcpy(it->payload, payload, len);
    it->length = len;

    gs_inTail = (uint8_t)((gs_inTail + 1U) % MTLX_INMSG_Q_DEPTH);
    gs_inCount++;
    return TRUE;
}

static BOOL inQueuePop(MtlxIncomingMsg_t *out)
{
    if (out == NULL || gs_inCount == 0U)
        return FALSE;

    *out = gs_inQ[gs_inHead];
    gs_inHead = (uint8_t)((gs_inHead + 1U) % MTLX_INMSG_Q_DEPTH);
    gs_inCount--;
    return TRUE;
}

/* ================================================================== */
/*                       TLV BUILDER                                  */
/* ================================================================== */

void mtlxBuilderInit(MtlxBuilder_t *b, uint8_t *buf, uint16_t capacity)
{
    b->buf      = buf;
    b->capacity = capacity;
    b->pos      = 0;
    b->overflow = FALSE;
}

void mtlxPacketBegin(MtlxBuilder_t *b)
{
    b->pos      = 0;
    b->overflow = FALSE;
    if (b->capacity < 2) { b->overflow = TRUE; return; }
    b->buf[b->pos++] = MTLX_PKT_START;
}

static void builderAddTLV(MtlxBuilder_t *b, uint16_t tag,
                           const uint8_t *val, uint16_t valLen)
{
    if (b->overflow) return;
    if ((uint32_t)b->pos + TLV_HEADER_SIZE + valLen + 1 > b->capacity)
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

void mtlxAddString(MtlxBuilder_t *b, uint16_t tag, const char *str)
{
    uint16_t len = (str != NULL) ? (uint16_t)strlen(str) : 0;
    builderAddTLV(b, tag, (const uint8_t *)str, len);
}

void mtlxAddBool(MtlxBuilder_t *b, uint16_t tag, BOOL val)
{
    uint8_t v = val ? 0x01 : 0x00;
    builderAddTLV(b, tag, &v, 1);
}

void mtlxAddUint8(MtlxBuilder_t *b, uint16_t tag, uint8_t val)
{
    builderAddTLV(b, tag, &val, 1);
}

void mtlxAddUint16(MtlxBuilder_t *b, uint16_t tag, uint16_t val)
{
    uint8_t tmp[2];
    writeU16BE(tmp, val);
    builderAddTLV(b, tag, tmp, 2);
}

void mtlxAddUint32(MtlxBuilder_t *b, uint16_t tag, uint32_t val)
{
    uint8_t tmp[4];
    tmp[0] = (uint8_t)(val >> 24);
    tmp[1] = (uint8_t)(val >> 16);
    tmp[2] = (uint8_t)(val >> 8);
    tmp[3] = (uint8_t)(val);
    builderAddTLV(b, tag, tmp, 4);
}

void mtlxAddRaw(MtlxBuilder_t *b, uint16_t tag, const uint8_t *data, uint16_t len)
{
    builderAddTLV(b, tag, data, len);
}

uint16_t mtlxPacketEnd(MtlxBuilder_t *b)
{
    if (b->overflow) return 0;
    if (b->pos >= b->capacity) { b->overflow = TRUE; return 0; }
    b->buf[b->pos++] = MTLX_PKT_END;
    return b->pos;
}

/* ================================================================== */
/*                         TLV PARSER                                 */
/* ================================================================== */

BOOL mtlxParserInit(MtlxParser_t *p, const uint8_t *raw, uint16_t rawLen)
{
    if (raw == NULL || rawLen < 2) return FALSE;

    uint16_t start = 0;
    while (start < rawLen && raw[start] != MTLX_PKT_START)
        start++;
    if (start >= rawLen) return FALSE;
    start++;

    uint16_t end = rawLen;
    for (uint16_t i = end; i > start; i--)
    {
        if (raw[i - 1] == MTLX_PKT_END) { end = i - 1; break; }
    }
    if (end <= start) return FALSE;

    p->data   = raw + start;
    p->length = (uint16_t)(end - start);
    p->cursor = 0;
    return TRUE;
}

void mtlxParserReset(MtlxParser_t *p)
{
    p->cursor = 0;
}

BOOL mtlxParserNext(MtlxParser_t *p, uint16_t *tag,
                    const uint8_t **value, uint16_t *valueLen)
{
    if (p->cursor + TLV_HEADER_SIZE > p->length) return FALSE;
    *tag      = readU16BE(p->data + p->cursor);       p->cursor += 2;
    *valueLen = readU16BE(p->data + p->cursor);       p->cursor += 2;
    if (p->cursor + *valueLen > p->length) { p->cursor = p->length; return FALSE; }
    *value = p->data + p->cursor;
    p->cursor += *valueLen;
    return TRUE;
}

static BOOL findTag(MtlxParser_t *p, uint16_t wantTag,
                    const uint8_t **value, uint16_t *valueLen)
{
    uint16_t saved = p->cursor;
    p->cursor = 0;

    uint16_t tag; const uint8_t *v; uint16_t vl;
    while (mtlxParserNext(p, &tag, &v, &vl))
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

BOOL mtlxGetString(MtlxParser_t *p, uint16_t tag, char *out, uint16_t outSz)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl)) return FALSE;
    uint16_t cp = (vl < outSz - 1) ? vl : (uint16_t)(outSz - 1);
    memcpy(out, v, cp);
    out[cp] = '\0';
    return TRUE;
}

BOOL mtlxGetBool(MtlxParser_t *p, uint16_t tag, BOOL *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = (v[0] != 0) ? TRUE : FALSE;
    return TRUE;
}

BOOL mtlxGetUint8(MtlxParser_t *p, uint16_t tag, uint8_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = v[0];
    return TRUE;
}

BOOL mtlxGetUint16(MtlxParser_t *p, uint16_t tag, uint16_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 2) return FALSE;
    *out = readU16BE(v);
    return TRUE;
}

BOOL mtlxGetUint32(MtlxParser_t *p, uint16_t tag, uint32_t *out)
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
    FsFile *f = fsOpenFile((char_t *)MTLX_REGISTER_FILE, FS_FILE_MODE_READ);
    if (f == NULL) return FALSE;
    U8 val = 0; size_t got = 0;
    (void)fsReadFile(f, &val, 1, &got);
    fsCloseFile(f);
    return (got == 1 && val == 1) ? TRUE : FALSE;
}

static void registerStateSave(BOOL reg)
{
    FsFile *f = fsOpenFile((char_t *)MTLX_REGISTER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL) return;
    U8 val = reg ? 1 : 0;
    (void)fsWriteFile(f, &val, 1);
    fsCloseFile(f);
}

/* ================================================================== */
/*                   Server-address persistence                       */
/* ================================================================== */

static BOOL serverConfigLoad(char *ip, size_t ipSz, int *port)
{
    ServerConfig_t cfg;
    FsFile *f = fsOpenFile((char_t *)MTLX_SERVER_FILE, FS_FILE_MODE_READ);
    if (f == NULL) return FALSE;
    size_t got = 0;
    (void)fsReadFile(f, &cfg, sizeof(cfg), &got);
    fsCloseFile(f);
    if (got != sizeof(cfg)) return FALSE;
    strncpy(ip, cfg.ip, ipSz - 1); ip[ipSz - 1] = '\0';
    *port = cfg.port;
    return TRUE;
}

static void serverConfigSave(const char *ip, int port)
{
    ServerConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ip, ip, sizeof(cfg.ip) - 1);
    cfg.port = port;
    FsFile *f = fsOpenFile((char_t *)MTLX_SERVER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL) return;
    (void)fsWriteFile(f, &cfg, sizeof(cfg));
    fsCloseFile(f);
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

static void addPacketHeader(MtlxBuilder_t *b, uint16_t transNum, uint8_t func)
{
    mtlxAddUint16(b, MTLX_TAG_TRANS_NUMBER, transNum);
    mtlxAddString(b, MTLX_TAG_FLAG, MTLX_FLAG);
    mtlxAddString(b, MTLX_TAG_SERIAL_NUMBER, gs_serialNumber);
    mtlxAddUint8(b, MTLX_TAG_FUNCTION, func);
}

static uint16_t buildAckNack(uint8_t *buf, uint16_t sz, BOOL isAck, uint16_t transNum)
{
    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, isAck ? MTLX_FUNC_ACK : MTLX_FUNC_NACK);
    mtlxAddBool(&b, MTLX_TAG_ACK_STATUS, isAck);
    return mtlxPacketEnd(&b);
}

static uint16_t buildIdentMsg(uint8_t *buf, uint16_t sz, uint16_t transNum)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, MTLX_FUNC_IDENT);
    mtlxAddBool(&b, MTLX_TAG_REGISTERED, gs_registered);
    mtlxAddString(&b, MTLX_TAG_DEVICE_BRAND, MTLX_BRAND);
    mtlxAddString(&b, MTLX_TAG_DEVICE_MODEL, MTLX_MODEL);
    mtlxAddString(&b, MTLX_TAG_DEVICE_DATE, dateStr);
    mtlxAddString(&b, MTLX_TAG_PULL_IP, gs_deviceIP);
    mtlxAddUint16(&b, MTLX_TAG_PULL_PORT, (uint16_t)gs_pullPort);
    return mtlxPacketEnd(&b);
}

static uint16_t buildAliveMsg(uint8_t *buf, uint16_t sz, uint16_t transNum)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, MTLX_FUNC_ALIVE);
    mtlxAddString(&b, MTLX_TAG_DEVICE_DATE, dateStr);
    return mtlxPacketEnd(&b);
}

static uint16_t buildLogPacket(uint8_t *buf, uint16_t sz,
                                const char *logData, int packetNum,
                                BOOL stream, uint16_t transNum)
{
    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, MTLX_FUNC_LOG);
    mtlxAddUint16(&b, MTLX_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, MTLX_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, MTLX_TAG_LOG_DATA, logData);
    return mtlxPacketEnd(&b);
}

static uint16_t buildDataPacket(uint8_t *buf, uint16_t sz,
                                 uint8_t func, const char *meterId,
                                 const char *data, int packetNum,
                                 BOOL stream, uint16_t transNum)
{
    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, func);
    mtlxAddUint16(&b, MTLX_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, MTLX_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, MTLX_TAG_METER_ID, meterId);
    mtlxAddString(&b, MTLX_TAG_READOUT_DATA, data);
    return mtlxPacketEnd(&b);
}

static uint16_t buildDirectivePacket(uint8_t *buf, uint16_t sz,
                                      const char *directive,
                                      int packetNum, BOOL stream,
                                      uint16_t transNum)
{
    MtlxBuilder_t b;
    mtlxBuilderInit(&b, buf, sz);
    mtlxPacketBegin(&b);
    addPacketHeader(&b, transNum, MTLX_FUNC_DIRECTIVE_LIST);
    mtlxAddUint16(&b, MTLX_TAG_PACKET_NUM, (uint16_t)packetNum);
    mtlxAddBool(&b, MTLX_TAG_PACKET_STREAM, stream);
    mtlxAddString(&b, MTLX_TAG_DIRECTIVE_DATA, directive);
    return mtlxPacketEnd(&b);
}

/* ================================================================== */
/*               Packetised send helpers                              */
/* ================================================================== */

static void sendLogPackets(uint16_t transNum)
{
    char curBuf[MTLX_DATA_CHUNK_SIZE + 1];
    char nxtBuf[MTLX_DATA_CHUNK_SIZE + 1];

    S32 curLen = appLogRecRead(g_sysLoggerID, curBuf, MTLX_DATA_CHUNK_SIZE);
    if (curLen <= 0)
    {
        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), "NO-LOG", 1, FALSE, transNum);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    int packetNum = 0;
    while (curLen > 0)
    {
        curBuf[curLen] = '\0';
        packetNum++;
        S32 nxtLen = appLogRecRead(g_sysLoggerID, nxtBuf, MTLX_DATA_CHUNK_SIZE);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), curBuf, packetNum, hasMore, transNum);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

        if (!hasMore) break;
        memcpy(curBuf, nxtBuf, (size_t)nxtLen);
        curLen = nxtLen;
    }
}

static void sendFilePacketised(const char *channel, uint8_t func,
                                const char *meterId, const char *filePath,
                                uint16_t transNum)
{
    FsFile *f = fsOpenFile((char_t *)filePath, FS_FILE_MODE_READ);
    if (f == NULL) return;

    char bufA[MTLX_DATA_CHUNK_SIZE + 1];
    char bufB[MTLX_DATA_CHUNK_SIZE + 1];
    char *cur = bufA, *nxt = bufB;

    size_t curLen = 0;
    (void)fsReadFile(f, cur, MTLX_DATA_CHUNK_SIZE, &curLen);
    if (curLen == 0) { fsCloseFile(f); return; }

    int packetNum = 0;
    while (curLen > 0)
    {
        cur[curLen] = '\0';
        packetNum++;
        size_t nxtLen = 0;
        (void)fsReadFile(f, nxt, MTLX_DATA_CHUNK_SIZE, &nxtLen);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildDataPacket(gs_txBuf, sizeof(gs_txBuf),
                                       func, meterId, cur, packetNum, hasMore, transNum);
        appTcpConnManagerSend(channel, (char *)gs_txBuf, (unsigned int)sz);

        if (!hasMore) break;
        char *tmp = cur; cur = nxt; nxt = tmp;
        curLen = nxtLen;
    }
    fsCloseFile(f);
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
    FsFile *f = fsOpenFile((char_t *)path, FS_FILE_MODE_READ);
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
        if (gs_pendingJobs[i].taskId == taskID)
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
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_readout.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_payload.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
}

static void deliverMeterData(int jobIdx, uint16_t transNum)
{
    PendingJob_t *job = &gs_pendingJobs[jobIdx];
    uint8_t func = job->isLoadProfile ? MTLX_FUNC_LOADPROFILE : MTLX_FUNC_READOUT;

    char meterId[128] = {0};
    char idPath[MAX_PATH_LEN];
    snprintf(idPath, sizeof(idPath), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);

    size_t got = 0;
    if (SUCCESS != readWholeText(idPath, meterId, sizeof(meterId), &got) || got == 0)
        strncpy(meterId, "unknown", sizeof(meterId));

    if (job->isLoadProfile)
        strncpy(meterId, "load-profile-no-id", sizeof(meterId));

    char dataPath[MAX_PATH_LEN];
    if (job->isLoadProfile)
        snprintf(dataPath, sizeof(dataPath), "%s%d_payload.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);
    else
        snprintf(dataPath, sizeof(dataPath), "%s%d_readout.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);

    if (!appTcpConnManagerIsConnectedPush())
    {
        appTcpConnManagerRequestPushConnect();
        U32 t0 = osGetSystemTime();
        while ((osGetSystemTime() - t0) < MTLX_CONNECT_TIMEOUT_MS)
        {
            if (appTcpConnManagerIsConnectedPush()) break;
            zosDelayTask(100);
        }
        if (!appTcpConnManagerIsConnectedPush())
        {
            DEBUG_ERROR("->[E] MtlxSess: push connect timeout, data delivery skipped");
            return;
        }
    }

    sendFilePacketised(PUSH_TCP_SOCK_NAME, func, meterId, dataPath, transNum);

    gs_pushRx.ready = FALSE;
    U32 t0 = osGetSystemTime();
    while ((osGetSystemTime() - t0) < MTLX_ACK_TIMEOUT_MS)
    {
        if (gs_pushRx.ready) { gs_pushRx.ready = FALSE; break; }
        zosDelayTask(100);
    }
}

/* ================================================================== */
/*           Session management                                       */
/* ================================================================== */

static void sessionDelete(MtlxSession_t *s)
{
    if (s != NULL)
        memset(s, 0, sizeof(MtlxSession_t));
}

static MtlxSession_t *sessionCreate(uint8_t function, const char *channel,
                                     uint16_t transNum)
{
    for (unsigned int i = 0; i < MTLX_SESSION_MAX; i++)
    {
        if (gs_sessionTable[i].inUse == FALSE)
        {
            MtlxSession_t *s = &gs_sessionTable[i];
            memset(s, 0, sizeof(*s));
            s->inUse            = TRUE;
            s->function         = function;
            s->transNumber      = transNum;
            s->step             = 0;
            s->timeoutGobackStep = 0;
            s->timeCnt          = 0;
            s->retryIntervalMs  = 0;
            s->retryCount       = MTLX_SESSION_RETRY_MAX;
            s->pendingJobIdx    = -1;
            s->rxReady          = FALSE;
            s->rxLen            = 0;

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

static MtlxSession_t *findSessionByTransNumber(uint16_t transNum)
{
    for (unsigned int i = 0; i < MTLX_SESSION_MAX; i++)
    {
        if (gs_sessionTable[i].inUse && gs_sessionTable[i].transNumber == transNum)
            return &gs_sessionTable[i];
    }
    return NULL;
}

static BOOL sessionSetIncoming(MtlxSession_t *s, const MtlxIncomingMsg_t *msg)
{
    if (s == NULL || msg == NULL)
        return FALSE;
    if (msg->length == 0 || msg->length > MTLX_PACKET_MAX_SIZE)
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

static BOOL sessionGetParser(MtlxSession_t *s, MtlxParser_t *p)
{
    if (s == NULL || p == NULL)
        return FALSE;
    if (!s->rxReady || s->rxLen == 0)
        return FALSE;
    if (!mtlxParserInit(p, s->rxBuf, s->rxLen))
        return FALSE;
    return TRUE;
}

static BOOL mtlxReceiveEvent(MtlxEventMsg_t *msg)
{
    if (msg == NULL || gs_evCount == 0U) return FALSE;
    *msg = gs_eventQ[gs_evHead];
    gs_evHead = (uint8_t)((gs_evHead + 1U) % MTLX_EVENT_Q_DEPTH);
    gs_evCount--;
    return TRUE;
}

static void checkAllSessionTimeouts(void)
{
    for (unsigned int i = 0; i < MTLX_SESSION_MAX; i++)
    {
        if (gs_sessionTable[i].inUse == FALSE) continue;

        MtlxSession_t *s = &gs_sessionTable[i];
        s->timeCnt += 1000U;

        if (s->retryIntervalMs == 0U) continue;
        if (s->timeCnt < s->retryIntervalMs) continue;

        s->retryCount--;
        if (s->retryCount == 0U)
        {
            DEBUG_WARNING("->[W] MtlxSess: session timeout (func 0x%02X, trans %u)",
                          s->function, s->transNumber);
            sessionDelete(s);
        }
        else
        {
            s->timeCnt = 0;
            s->step = s->timeoutGobackStep;
        }
    }
}

static void periodicTimerCb(void)
{
    checkAllSessionTimeouts();

    gs_aliveCnt++;
    if (gs_aliveCnt >= (uint32_t)MTLX_ALIVE_INTERVAL_S)
    {
        gs_sendAlive = TRUE;
        gs_aliveCnt  = 0;
    }
}

/* ================================================================== */
/*          Session handlers — step-based state machines              */
/* ================================================================== */

/* ── Push Ident (registration) ── */
static void processIdentSession(MtlxSession_t *session)
{
    switch (session->step)
    {
        case 0:
            appTcpConnManagerRequestPushConnect();
            session->retryIntervalMs   = MTLX_CONNECT_TIMEOUT_MS;
            session->retryCount        = MTLX_SESSION_RETRY_MAX;
            session->timeoutGobackStep = 0;
            session->timeCnt           = 0;
            session->step              = 1;
            break;

        case 1:
            if (appTcpConnManagerIsConnectedPush())
            {
                session->timeCnt = 0;
                session->step    = 2;
            }
            break;

        case 2:
        {
            uint16_t sz = buildIdentMsg(gs_txBuf, sizeof(gs_txBuf), session->transNumber);
            appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            DEBUG_DEBUG("->[D] MtlxSess: ident sent (trans %u, %u bytes)",
                        session->transNumber, (unsigned)sz);

            session->retryIntervalMs   = (uint32_t)MTLX_REGISTER_RETRY_S * 1000U;
            session->retryCount        = MTLX_SESSION_RETRY_MAX;
            session->timeoutGobackStep = 0;
            session->timeCnt           = 0;
            session->step              = 3;
            break;
        }

        case 3:
            if (session->rxReady)
            {
                MtlxParser_t rp;
                if (sessionGetParser(session, &rp))
                {
                    uint16_t rxTrans = 0;
                    mtlxGetUint16(&rp, MTLX_TAG_TRANS_NUMBER, &rxTrans);

                    if (rxTrans == session->transNumber)
                    {
                        uint8_t rFunc = 0;
                        if (mtlxGetUint8(&rp, MTLX_TAG_FUNCTION, &rFunc)
                            && rFunc == MTLX_FUNC_IDENT)
                        {
                            BOOL regVal = FALSE;
                            if (mtlxGetBool(&rp, MTLX_TAG_REGISTER, &regVal) && regVal)
                            {
                                gs_registered = TRUE;
                                registerStateSave(TRUE);
                                DEBUG_INFO("->[I] MtlxSess: device registered");
                                APP_LOG_REC(g_sysLoggerID, "Device registered");
                            }
                        }
                    }
                }
                session->rxReady = FALSE;
                session->rxLen = 0;

                if (gs_registered)
                {
                    sessionDelete(session);
                }
                else
                {
                    appTcpConnManagerRequestPushDisconnect();
                    session->step    = 0;
                    session->timeCnt = 0;
                }
            }
            break;

        default:
            sessionDelete(session);
            break;
    }
}

/* ── Alive ── */
static void processAliveSession(MtlxSession_t *session)
{
    if (appTcpConnManagerIsConnectedPush())
    {
        uint16_t sz = buildAliveMsg(gs_txBuf, sizeof(gs_txBuf), session->transNumber);
        appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        DEBUG_DEBUG("->[D] MtlxSess: alive sent (trans %u)", session->transNumber);
    }
    sessionDelete(session);
}

/* ── Pull-socket request handlers ── */

static void processLogSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: log request (trans %u)", session->transNumber);
    sendLogPackets(session->transNumber);
    sessionDelete(session);
}

static void processSettingSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: setting request (trans %u)", session->transNumber);
    MtlxParser_t parser;
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
    BOOL hasIP   = mtlxGetString(&parser, MTLX_TAG_SERVER_IP, newIP, sizeof(newIP));
    BOOL hasPort = mtlxGetUint16(&parser, MTLX_TAG_SERVER_PORT, &newPort);

    if (hasIP && newIP[0] != '\0' && hasPort && newPort > 0)
    {
        strncpy(gs_serverIP, newIP, sizeof(gs_serverIP) - 1);
        gs_serverPort = (int)newPort;
        serverConfigSave(gs_serverIP, gs_serverPort);

        appTcpConnManagerStop();
        zosDelayTask(500);
        appTcpConnManagerStart(gs_serverIP, gs_serverPort,
                               gs_pullPort, appProtocolMetalixPutIncomingMessage);
        appTcpConnManagerRequestConnect();

        DEBUG_INFO("->[I] MtlxSess: server updated to %s:%d", gs_serverIP, gs_serverPort);
        APP_LOG_REC(g_sysLoggerID, "Server address updated");
    }

    mtlxParserReset(&parser);

    uint16_t tag; const uint8_t *val; uint16_t vLen;
    BOOL inMeter = FALSE;
    char operation[16] = {0};
    MeterData_t md;

    while (mtlxParserNext(&parser, &tag, &val, &vLen))
    {
        if (tag == MTLX_TAG_METER_INDEX)
        {
            if (inMeter)
            {
                if (0 == strcmp(operation, "add"))
                { if (SUCCESS != appMeterOperationsAddMeter(&md)) success = FALSE; }
                else if (0 == strcmp(operation, "remove"))
                { if (md.serialNumber[0] != '\0') if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber)) success = FALSE; }
            }
            inMeter = TRUE;
            memset(&md, 0, sizeof(md));
            memset(operation, 0, sizeof(operation));
            continue;
        }
        if (!inMeter) continue;

        switch (tag)
        {
            case MTLX_TAG_METER_OPERATION:
            { uint16_t cp = vLen < sizeof(operation)-1 ? vLen : (uint16_t)(sizeof(operation)-1); memcpy(operation, val, cp); operation[cp]='\0'; break; }
            case MTLX_TAG_METER_PROTOCOL:
            { uint16_t cp = vLen < sizeof(md.protocol)-1 ? vLen : (uint16_t)(sizeof(md.protocol)-1); memcpy(md.protocol, val, cp); md.protocol[cp]='\0'; break; }
            case MTLX_TAG_METER_TYPE:
            { uint16_t cp = vLen < sizeof(md.type)-1 ? vLen : (uint16_t)(sizeof(md.type)-1); memcpy(md.type, val, cp); md.type[cp]='\0'; break; }
            case MTLX_TAG_METER_BRAND:
            { uint16_t cp = vLen < sizeof(md.brand)-1 ? vLen : (uint16_t)(sizeof(md.brand)-1); memcpy(md.brand, val, cp); md.brand[cp]='\0'; break; }
            case MTLX_TAG_METER_SERIAL_NUM:
            { uint16_t cp = vLen < sizeof(md.serialNumber)-1 ? vLen : (uint16_t)(sizeof(md.serialNumber)-1); memcpy(md.serialNumber, val, cp); md.serialNumber[cp]='\0'; break; }
            case MTLX_TAG_METER_SERIAL_PORT:
            { uint16_t cp = vLen < sizeof(md.serialPort)-1 ? vLen : (uint16_t)(sizeof(md.serialPort)-1); memcpy(md.serialPort, val, cp); md.serialPort[cp]='\0'; break; }
            case MTLX_TAG_METER_INIT_BAUD:
            { if (vLen >= 4) md.initBaud = (int)(((uint32_t)val[0]<<24)|((uint32_t)val[1]<<16)|((uint32_t)val[2]<<8)|val[3]); break; }
            case MTLX_TAG_METER_FIX_BAUD:
                md.fixBaud = (vLen >= 1 && val[0] != 0) ? TRUE : FALSE; break;
            case MTLX_TAG_METER_FRAME:
            { uint16_t cp = vLen < sizeof(md.frame)-1 ? vLen : (uint16_t)(sizeof(md.frame)-1); memcpy(md.frame, val, cp); md.frame[cp]='\0'; break; }
            case MTLX_TAG_METER_CUSTOMER_NUM:
            { uint16_t cp = vLen < sizeof(md.customerNumber)-1 ? vLen : (uint16_t)(sizeof(md.customerNumber)-1); memcpy(md.customerNumber, val, cp); md.customerNumber[cp]='\0'; break; }
            default: break;
        }
    }

    if (inMeter)
    {
        if (0 == strcmp(operation, "add"))
        { if (SUCCESS != appMeterOperationsAddMeter(&md)) success = FALSE; }
        else if (0 == strcmp(operation, "remove"))
        { if (md.serialNumber[0] != '\0') if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber)) success = FALSE; }
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), success, session->transNumber);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

static void processFwUpdateSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: fwUpdate request (trans %u)", session->transNumber);
    MtlxParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char address[192] = {0};
    if (!mtlxGetString(&parser, MTLX_TAG_FW_ADDRESS, address, sizeof(address)) || address[0] == '\0')
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
        AppSwUpdateInit(host, port, user, pass, path, FW_LOCAL_PATH);
        DEBUG_INFO("->[I] MtlxSess: FW update started from %s", host);
        APP_LOG_REC(g_sysLoggerID, "FW update started");
    }

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

/* ── Readout — multi-step ── */
static void processReadoutSession(MtlxSession_t *session)
{
    MtlxParser_t parser;

    switch (session->step)
    {
        case 0:
        {
            DEBUG_INFO("->[I] MtlxSess: readout request (trans %u)", session->transNumber);
            if (!sessionGetParser(session, &parser))
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session);
                return;
            }
            char directive[64] = {0}, meterSn[20] = {0};
            mtlxGetString(&parser, MTLX_TAG_DIRECTIVE_NAME, directive, sizeof(directive));
            mtlxGetString(&parser, MTLX_TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));

            char reqBuf[320];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\"}}",
                     directive, meterSn);

            S32 tid = appMeterOperationsAddReadoutTask(reqBuf, meterTaskCallback);
            if (tid <= 0)
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            int slot = pendingJobAllocSlot(tid, FALSE);
            if (slot < 0)
            {
                appMeterOperationsTaskIDFree(tid);
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            session->pendingJobIdx     = slot;
            session->retryIntervalMs   = 120000U;
            session->retryCount        = 1;
            session->timeoutGobackStep = 1;
            session->timeCnt           = 0;

            uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            session->step = 1;
            session->rxReady = FALSE;
            session->rxLen = 0;
            break;
        }

        case 1:
            if (session->pendingJobIdx >= 0 &&
                gs_pendingJobs[session->pendingJobIdx].completed)
            {
                if (gs_pendingJobs[session->pendingJobIdx].result == EN_ERR_CODE_SUCCESS)
                    deliverMeterData(session->pendingJobIdx, session->transNumber);
                else
                    DEBUG_WARNING("->[W] MtlxSess: readout task failed (err %d)",
                                  gs_pendingJobs[session->pendingJobIdx].result);

                cleanupMeterFiles(gs_pendingJobs[session->pendingJobIdx].taskId);
                appMeterOperationsTaskIDFree(gs_pendingJobs[session->pendingJobIdx].taskId);
                gs_pendingJobs[session->pendingJobIdx].taskId    = 0;
                gs_pendingJobs[session->pendingJobIdx].completed = FALSE;

                sessionDelete(session);
            }
            break;

        default:
            sessionDelete(session);
            break;
    }
}

/* ── Load Profile — multi-step ── */
static void processLoadProfileSession(MtlxSession_t *session)
{
    MtlxParser_t parser;

    switch (session->step)
    {
        case 0:
        {
            DEBUG_INFO("->[I] MtlxSess: loadprofile request (trans %u)", session->transNumber);
            if (!sessionGetParser(session, &parser))
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session);
                return;
            }
            char directive[64] = {0}, meterSn[20] = {0};
            char startDate[24] = {0}, endDate[24] = {0};

            mtlxGetString(&parser, MTLX_TAG_DIRECTIVE_NAME, directive, sizeof(directive));
            mtlxGetString(&parser, MTLX_TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));
            mtlxGetString(&parser, MTLX_TAG_START_DATE, startDate, sizeof(startDate));
            mtlxGetString(&parser, MTLX_TAG_END_DATE, endDate, sizeof(endDate));

            char reqBuf[384];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\""
                     ",\"startDate\":\"%s\",\"endDate\":\"%s\"}}",
                     directive, meterSn, startDate, endDate);

            S32 tid = appMeterOperationsAddProfileTask(reqBuf, meterTaskCallback);
            if (tid <= 0)
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            int slot = pendingJobAllocSlot(tid, TRUE);
            if (slot < 0)
            {
                appMeterOperationsTaskIDFree(tid);
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                sessionDelete(session); return;
            }

            session->pendingJobIdx     = slot;
            session->retryIntervalMs   = 120000U;
            session->retryCount        = 1;
            session->timeoutGobackStep = 1;
            session->timeCnt           = 0;

            uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            session->step = 1;
            session->rxReady = FALSE;
            session->rxLen = 0;
            break;
        }

        case 1:
            if (session->pendingJobIdx >= 0 &&
                gs_pendingJobs[session->pendingJobIdx].completed)
            {
                if (gs_pendingJobs[session->pendingJobIdx].result == EN_ERR_CODE_SUCCESS)
                    deliverMeterData(session->pendingJobIdx, session->transNumber);
                else
                    DEBUG_WARNING("->[W] MtlxSess: LP task failed (err %d)",
                                  gs_pendingJobs[session->pendingJobIdx].result);

                cleanupMeterFiles(gs_pendingJobs[session->pendingJobIdx].taskId);
                appMeterOperationsTaskIDFree(gs_pendingJobs[session->pendingJobIdx].taskId);
                gs_pendingJobs[session->pendingJobIdx].taskId    = 0;
                gs_pendingJobs[session->pendingJobIdx].completed = FALSE;

                sessionDelete(session);
            }
            break;

        default:
            sessionDelete(session);
            break;
    }
}

static void processDirectiveListSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: directiveList request (trans %u)", session->transNumber);
    MtlxParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char filterId[64] = {0};
    mtlxGetString(&parser, MTLX_TAG_DIRECTIVE_ID, filterId, sizeof(filterId));

    BOOL sendAll = (filterId[0] == '\0' || (filterId[0] == '*' && filterId[1] == '\0'));

    U32 count = appMeterOperationsGetDirectiveCount();
    if (count == 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session); return;
    }

    char dirBuf[MTLX_PACKET_MAX_SIZE];
    int totalToSend = 0;
    if (sendAll) totalToSend = (int)count;
    else { if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf))) totalToSend = 1; }

    if (totalToSend == 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE, session->transNumber);
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
            uint16_t sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf,
                                               packetNum, (sent < totalToSend), session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
        {
            uint16_t sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf,
                                               1, FALSE, session->transNumber);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }

    session->rxReady = FALSE;
    session->rxLen = 0;
    sessionDelete(session);
}

static void processDirectiveAddSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: directiveAdd request (trans %u)", session->transNumber);
    MtlxParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char dirObj[MTLX_PACKET_MAX_SIZE];
    if (!mtlxGetString(&parser, MTLX_TAG_DIRECTIVE_DATA, dirObj, sizeof(dirObj)))
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

static void processDirectiveDeleteSession(MtlxSession_t *session)
{
    DEBUG_INFO("->[I] MtlxSess: directiveDelete request (trans %u)", session->transNumber);
    MtlxParser_t parser;
    if (!sessionGetParser(session, &parser))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, session->transNumber);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        sessionDelete(session);
        return;
    }

    char filterId[64] = {0};
    mtlxGetString(&parser, MTLX_TAG_DIRECTIVE_ID, filterId, sizeof(filterId));

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

static void processSessionMessage(MtlxSession_t *session)
{
    switch (session->function)
    {
        case SFUNC_PUSH_IDENT:    processIdentSession(session);           break;
        case SFUNC_ALIVE:         processAliveSession(session);           break;
        case MTLX_FUNC_LOG:       processLogSession(session);            break;
        case MTLX_FUNC_SETTING:   processSettingSession(session);        break;
        case MTLX_FUNC_FW_UPDATE: processFwUpdateSession(session);       break;
        case MTLX_FUNC_READOUT:   processReadoutSession(session);        break;
        case MTLX_FUNC_LOADPROFILE: processLoadProfileSession(session);  break;
        case MTLX_FUNC_DIRECTIVE_LIST: processDirectiveListSession(session); break;
        case MTLX_FUNC_DIRECTIVE_ADD:  processDirectiveAddSession(session);  break;
        case MTLX_FUNC_DIRECTIVE_DEL:  processDirectiveDeleteSession(session); break;
        default:
            DEBUG_WARNING("->[W] MtlxSess: unknown func 0x%02X", session->function);
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

static void dispatchIncomingMessage(const MtlxIncomingMsg_t *msg)
{
    if (msg == NULL || msg->length == 0)
        return;

    MtlxParser_t parser;
    if (!mtlxParserInit(&parser, msg->payload, msg->length))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, 0);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    uint8_t func = 0;
    uint16_t transNum = 0;

    if (!mtlxGetUint8(&parser, MTLX_TAG_FUNCTION, &func))
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, 0);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }
    mtlxGetUint16(&parser, MTLX_TAG_TRANS_NUMBER, &transNum);

    /* if transNum belongs to an active session, route it */
    MtlxSession_t *session = (transNum != 0) ? findSessionByTransNumber(transNum) : NULL;
    if (session)
    {
        (void)sessionSetIncoming(session, msg);
        processSessionMessage(session);
        return;
    }

    /* start new session */
    const char *ch = (msg->ch[0] != '\0') ? msg->ch : PULL_TCP_SOCK_NAME;
    session = sessionCreate(func, ch, transNum);
    if (session)
    {
        (void)sessionSetIncoming(session, msg);
        processSessionMessage(session);
    }
    else
    {
        DEBUG_WARNING("->[W] MtlxSess: too many sessions");
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE, transNum);
        appTcpConnManagerSend(ch, (char *)gs_txBuf, (unsigned int)sz);
    }
}

static void handleEvent(MtlxEventMsg_t *evt)
{
    switch (evt->type)
    {
        case MTLX_EVENT_FW_UPGRADE_SUCCESS:
            gs_sendIdent = TRUE;
            break;

        case MTLX_EVENT_ALIVE:
            gs_sendAlive = TRUE;
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
    DEBUG_INFO("->[I] %s task started", MTLX_TASK_NAME);
    appTskMngImOK(gs_taskId);

    if (0 != appTcpConnManagerStart(gs_serverIP, gs_serverPort,
                                     gs_pullPort, appProtocolMetalixPutIncomingMessage))
    {
        DEBUG_ERROR("->[E] MtlxSess: TCP start failed");
        APP_LOG_REC(g_sysLoggerID, "Metalix: TCP start fail");
        gs_running = FALSE;
        return;
    }
    appTcpConnManagerRequestConnect();
    zosDelayTask(500);

    gs_registered = registerStateLoad();

    if (!gs_registered)
    {
        DEBUG_INFO("->[I] MtlxSess: not registered, starting ident session");
        gs_sendIdent = TRUE;
    }
    else
    {
        DEBUG_INFO("->[I] MtlxSess: already registered");
    }

    while (gs_running)
    {
        appTskMngImOK(gs_taskId);

        /* 1. Incoming messages (push + pull) */
        {
            MtlxIncomingMsg_t in;
            while (inQueuePop(&in))
                dispatchIncomingMessage(&in);
        }

        /* 2. Events */
        {
            MtlxEventMsg_t evMsg;
            while (mtlxReceiveEvent(&evMsg))
                handleEvent(&evMsg);
        }

        /* 3. Process all active sessions */
        for (unsigned int i = 0; i < MTLX_SESSION_MAX; i++)
        {
            if (gs_sessionTable[i].inUse)
                processSessionMessage(&gs_sessionTable[i]);
        }

        /* 4. Push connect maintenance */
        if (gs_registered && !appTcpConnManagerIsConnectedPush())
            appTcpConnManagerRequestPushConnect();

        /* 5. Task flags */
        if (gs_sendIdent)
        {
            if (sessionCreate(SFUNC_PUSH_IDENT, PUSH_TCP_SOCK_NAME, nextTransNumber()) != NULL)
                gs_sendIdent = FALSE;
        }

        if (gs_sendAlive && gs_registered)
        {
            if (sessionCreate(SFUNC_ALIVE, PUSH_TCP_SOCK_NAME, nextTransNumber()) != NULL)
                gs_sendAlive = FALSE;
        }

        zosDelayTask(100);
    }

    appTcpConnManagerStop();
    DEBUG_INFO("->[I] %s task stopped", MTLX_TASK_NAME);
}

/* ================================================================== */
/*                       PUBLIC FUNCTIONS                             */
/* ================================================================== */

RETURN_STATUS appProtocolMetalixInit(const char *serialNumber,
                                     const char *serverIP, int serverPort,
                                     const char *deviceIP, int pullPort)
{
    if (serialNumber == NULL || serverIP == NULL || deviceIP == NULL)
        return FAILURE;

    memset(gs_serialNumber, 0, sizeof(gs_serialNumber));
    strncpy(gs_serialNumber, serialNumber, sizeof(gs_serialNumber) - 1);

    memset(gs_deviceIP, 0, sizeof(gs_deviceIP));
    strncpy(gs_deviceIP, deviceIP, sizeof(gs_deviceIP) - 1);

    gs_pullPort = pullPort;

    if (!serverConfigLoad(gs_serverIP, sizeof(gs_serverIP), &gs_serverPort))
    {
        strncpy(gs_serverIP, serverIP, sizeof(gs_serverIP) - 1);
        gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
        gs_serverPort = serverPort;
    }

    memset(gs_pendingJobs, 0, sizeof(gs_pendingJobs));
    memset(gs_sessionTable, 0, sizeof(gs_sessionTable));

    gs_inHead = gs_inTail = gs_inCount = 0;

    gs_evHead = gs_evTail = gs_evCount = 0;

    gs_registered = FALSE;
    gs_running    = FALSE;
    gs_taskId     = OS_INVALID_TASK_ID;
    gs_periodicTimerId = -1;
    gs_sendIdent  = FALSE;
    gs_sendAlive  = FALSE;
    gs_aliveCnt   = 0;
    gs_localTransNum = 0;

    return SUCCESS;
}

RETURN_STATUS appProtocolMetalixStart(void)
{
    if (gs_running) return SUCCESS;

    gs_running = TRUE;

    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = MTLX_TASK_STACK_SIZE;

    gs_taskId = appTskMngCreate(MTLX_TASK_NAME, protocolTaskFunc, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_taskId)
    {
        gs_running = FALSE;
        DEBUG_ERROR("->[E] MtlxSess: task creation failed");
        APP_LOG_REC(g_sysLoggerID, "Metalix task creation failed");
        return FAILURE;
    }

    if (gs_periodicTimerId < 0)
    {
        if (middEventTimerRegister(&gs_periodicTimerId, periodicTimerCb,
                                   1000U, TRUE) != SUCCESS)
        {
            DEBUG_WARNING("->[W] MtlxSess: periodic timer register failed");
        }
    }
    if (gs_periodicTimerId >= 0)
        (void)middEventTimerStart(gs_periodicTimerId);

    DEBUG_INFO("->[I] MtlxSess: started");
    APP_LOG_REC(g_sysLoggerID, "Protocol Metalix started");
    return SUCCESS;
}

RETURN_STATUS appProtocolMetalixStop(void)
{
    if (!gs_running) return SUCCESS;

    if (gs_periodicTimerId >= 0)
    {
        (void)middEventTimerStop(gs_periodicTimerId);
        (void)middEventTimerDelete(gs_periodicTimerId);
        gs_periodicTimerId = -1;
    }

    gs_running = FALSE;
    zosDelayTask(500);

    if (OS_INVALID_TASK_ID != gs_taskId)
    {
        appTskMngDelete(gs_taskId);
        gs_taskId = OS_INVALID_TASK_ID;
    }

    DEBUG_INFO("->[I] MtlxSess: stopped");
    APP_LOG_REC(g_sysLoggerID, "Protocol Metalix stopped");
    return SUCCESS;
}

void appProtocolMetalixPutIncomingMessage(const char *channel,
                                          const char *data,
                                          unsigned int dataLength)
{
    if (channel == NULL || data == NULL || dataLength == 0)
        return;

    (void)inQueuePush(channel, (const uint8_t *)data, (uint16_t)dataLength);
}

BOOL appProtocolMetalixSendEvent(MtlxEventType_t type)
{
    if (gs_evCount >= MTLX_EVENT_Q_DEPTH)
        return FALSE;
    gs_eventQ[gs_evTail].type = type;
    gs_evTail = (uint8_t)((gs_evTail + 1U) % MTLX_EVENT_Q_DEPTH);
    gs_evCount++;
    return TRUE;
}

/********************************* End Of File ********************************/
