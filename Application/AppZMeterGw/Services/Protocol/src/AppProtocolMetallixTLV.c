/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 29, 2026
* #File Name    : AppProtocolMetallixTLV.c
*******************************************************************************/
/******************************************************************************
* Binary TLV-over-TCP protocol handler for IoT meter gateway.
*
* State machine overview
* ──────────────────────
*  INIT  ─► connect push ─► send ident (registered=false)
*        ─► wait ACK (30 s) ─► if register==true ─► REGISTERED
*                              else retry after 30 s
*
*  REGISTERED  ─► alive every 5 min via push
*              ─► handle pull requests  (log, setting, fwUpdate,
*                  readout, loadprofile, directive*)
*              ─► deliver completed readout / loadprofile data via push
******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppProtocolMetallixTLV.h"
#include "AppTcpConnManager.h"
#include "AppMeterOperations.h"
#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppTimeService.h"
#include "AppSWUpdate.h"
#include "fs_port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/****************************** MACRO DEFINITIONS *****************************/
#define METALLIX_TASK_STACK_SIZE    (4096U)
#define METALLIX_TASK_NAME          "MetallixTLV"

#define MAX_PENDING_JOBS      (4)
#define MAX_PATH_LEN          (96)

#define FTP_FIELD_MAX         (64)
#define FTP_DEFAULT_PORT      (21)
#define FW_LOCAL_PATH         "/tmp/firmware.bin"

#define TLV_HEADER_SIZE       (4)   /* 2 tag + 2 len */

/******************************* TYPE DEFINITIONS *****************************/

typedef struct
{
    uint8_t data[PROTO_METLLX_PACKET_MAX_SIZE + 1];
    unsigned int length;
    volatile BOOL ready;
} RxBuffer_t;

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

/********************************** VARIABLES *********************************/

static char gs_serialNumber[20];
static char gs_serverIP[20];
static int  gs_serverPort;
static char gs_deviceIP[20];
static int  gs_pullPort;

static BOOL gs_registered = FALSE;
static volatile BOOL gs_running = FALSE;
static OsTaskId gs_taskId = OS_INVALID_TASK_ID;

static RxBuffer_t gs_pushRx;
static RxBuffer_t gs_pullRx;

static PendingJob_t gs_pendingJobs[MAX_PENDING_JOBS];

static uint8_t gs_txBuf[PROTO_METLLX_PACKET_MAX_SIZE];

/* ================================================================== */
/*                       TLV BUILDER                                  */
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

void metallixTLV_BuilderInit(MetallixTLV_Builder_t *b, uint8_t *buf, uint16_t capacity)
{
    b->buf      = buf;
    b->capacity = capacity;
    b->pos      = 0;
    b->overflow = FALSE;
}

void metallixTLV_PacketBegin(MetallixTLV_Builder_t *b)
{
    b->pos      = 0;
    b->overflow = FALSE;
    if (b->capacity < 2) { b->overflow = TRUE; return; }
    b->buf[b->pos++] = PROTO_METLLX_PKT_START;
}

static void builderAddTLV(MetallixTLV_Builder_t *b, uint16_t tag,
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

void metallixTLV_AddString(MetallixTLV_Builder_t *b, uint16_t tag, const char *str)
{
    uint16_t len = (str != NULL) ? (uint16_t)strlen(str) : 0;
    builderAddTLV(b, tag, (const uint8_t *)str, len);
}

void metallixTLV_AddStringN(MetallixTLV_Builder_t *b, uint16_t tag, const char *str, uint16_t len)
{
    builderAddTLV(b, tag, (const uint8_t *)str, len);
}

void metallixTLV_AddBool(MetallixTLV_Builder_t *b, uint16_t tag, BOOL val)
{
    uint8_t v = val ? 0x01 : 0x00;
    builderAddTLV(b, tag, &v, 1);
}

void metallixTLV_AddUint8(MetallixTLV_Builder_t *b, uint16_t tag, uint8_t val)
{
    builderAddTLV(b, tag, &val, 1);
}

void metallixTLV_AddUint16(MetallixTLV_Builder_t *b, uint16_t tag, uint16_t val)
{
    uint8_t tmp[2];
    writeU16BE(tmp, val);
    builderAddTLV(b, tag, tmp, 2);
}

void metallixTLV_AddUint32(MetallixTLV_Builder_t *b, uint16_t tag, uint32_t val)
{
    uint8_t tmp[4];
    tmp[0] = (uint8_t)(val >> 24);
    tmp[1] = (uint8_t)(val >> 16);
    tmp[2] = (uint8_t)(val >> 8);
    tmp[3] = (uint8_t)(val);
    builderAddTLV(b, tag, tmp, 4);
}

void metallixTLV_AddInt16(MetallixTLV_Builder_t *b, uint16_t tag, int16_t val)
{
    metallixTLV_AddUint16(b, tag, (uint16_t)val);
}

void metallixTLV_AddRaw(MetallixTLV_Builder_t *b, uint16_t tag,
                     const uint8_t *data, uint16_t len)
{
    builderAddTLV(b, tag, data, len);
}

uint16_t metallixTLV_PacketEnd(MetallixTLV_Builder_t *b)
{
    if (b->overflow) return 0;
    if (b->pos >= b->capacity) { b->overflow = TRUE; return 0; }
    b->buf[b->pos++] = PROTO_METLLX_PKT_END;
    return b->pos;
}

void metallixTLV_AddDeviceHeader(MetallixTLV_Builder_t *b)
{
    metallixTLV_AddString(b, TAG_FLAG, PROTO_METLLX_FLAG);
    metallixTLV_AddString(b, TAG_SERIAL_NUMBER, gs_serialNumber);
}

/* ================================================================== */
/*                         TLV PARSER                                 */
/* ================================================================== */

BOOL metallixTLV_ParserInit(MetallixTLV_Parser_t *p, const uint8_t *raw, uint16_t rawLen)
{
    if (raw == NULL || rawLen < 2) return FALSE;

    /* find '$' */
    uint16_t start = 0;
    while (start < rawLen && raw[start] != PROTO_METLLX_PKT_START)
        start++;

    if (start >= rawLen) return FALSE;
    start++; /* skip '$' */

    /* find '#' by scanning TLV chains from start */
    uint16_t end = rawLen;
    for (uint16_t i = end; i > start; i--)
    {
        if (raw[i - 1] == PROTO_METLLX_PKT_END) { end = i - 1; break; }
    }
    if (end <= start) return FALSE;

    p->data   = raw + start;
    p->length = (uint16_t)(end - start);
    p->cursor = 0;
    return TRUE;
}

void metallixTLV_ParserReset(MetallixTLV_Parser_t *p)
{
    p->cursor = 0;
}

BOOL metallixTLV_ParserNext(MetallixTLV_Parser_t *p, uint16_t *tag,
                         const uint8_t **value, uint16_t *valueLen)
{
    if (p->cursor + TLV_HEADER_SIZE > p->length)
        return FALSE;

    *tag      = readU16BE(p->data + p->cursor);       p->cursor += 2;
    *valueLen = readU16BE(p->data + p->cursor);       p->cursor += 2;

    if (p->cursor + *valueLen > p->length)
    {
        p->cursor = p->length;
        return FALSE;
    }
    *value = p->data + p->cursor;
    p->cursor += *valueLen;
    return TRUE;
}

/* Search by tag (resets cursor, scans all TLVs) */
static BOOL findTag(MetallixTLV_Parser_t *p, uint16_t wantTag,
                     const uint8_t **value, uint16_t *valueLen)
{
    uint16_t savedCursor = p->cursor;
    p->cursor = 0;

    uint16_t tag;
    const uint8_t *v;
    uint16_t vl;
    while (metallixTLV_ParserNext(p, &tag, &v, &vl))
    {
        if (tag == wantTag)
        {
            if (value)    *value    = v;
            if (valueLen) *valueLen = vl;
            p->cursor = savedCursor;
            return TRUE;
        }
    }
    p->cursor = savedCursor;
    return FALSE;
}

BOOL metallixTLV_GetString(MetallixTLV_Parser_t *p, uint16_t tag, char *out, uint16_t outSz)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl)) return FALSE;
    uint16_t cp = (vl < outSz - 1) ? vl : (uint16_t)(outSz - 1);
    memcpy(out, v, cp);
    out[cp] = '\0';
    return TRUE;
}

BOOL metallixTLV_GetBool(MetallixTLV_Parser_t *p, uint16_t tag, BOOL *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = (v[0] != 0) ? TRUE : FALSE;
    return TRUE;
}

BOOL metallixTLV_GetUint8(MetallixTLV_Parser_t *p, uint16_t tag, uint8_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 1) return FALSE;
    *out = v[0];
    return TRUE;
}

BOOL metallixTLV_GetUint16(MetallixTLV_Parser_t *p, uint16_t tag, uint16_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 2) return FALSE;
    *out = readU16BE(v);
    return TRUE;
}

BOOL metallixTLV_GetUint32(MetallixTLV_Parser_t *p, uint16_t tag, uint32_t *out)
{
    const uint8_t *v; uint16_t vl;
    if (!findTag(p, tag, &v, &vl) || vl < 4) return FALSE;
    *out = ((uint32_t)v[0] << 24) | ((uint32_t)v[1] << 16) |
           ((uint32_t)v[2] << 8)  | v[3];
    return TRUE;
}

BOOL metallixTLV_GetInt16(MetallixTLV_Parser_t *p, uint16_t tag, int16_t *out)
{
    uint16_t tmp;
    if (!metallixTLV_GetUint16(p, tag, &tmp)) return FALSE;
    *out = (int16_t)tmp;
    return TRUE;
}

BOOL metallixTLV_GetFunction(MetallixTLV_Parser_t *p, MetallixTLV_Function_t *out)
{
    uint8_t v;
    if (!metallixTLV_GetUint8(p, TAG_FUNCTION, &v)) return FALSE;
    *out = (MetallixTLV_Function_t)v;
    return TRUE;
}

/* ================================================================== */
/*                   Register-state persistence                       */
/* ================================================================== */

static BOOL registerStateLoad(void)
{
    FsFile *f = fsOpenFile((char_t *)PROTO_METLLX_REGISTER_FILE, FS_FILE_MODE_READ);
    if (f == NULL) return FALSE;

    U8 val = 0;
    size_t got = 0;
    (void)fsReadFile(f, &val, 1, &got);
    fsCloseFile(f);
    return (got == 1 && val == 1) ? TRUE : FALSE;
}

static void registerStateSave(BOOL reg)
{
    FsFile *f = fsOpenFile((char_t *)PROTO_METLLX_REGISTER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL) return;
    U8 val = reg ? 1 : 0;
    (void)fsWriteFile(f, &val, 1);
    fsCloseFile(f);
}

/* ================================================================== */
/*                   Server-address persistence                       */
/* ================================================================== */
static void connConfigDefault(void)
{
    strncpy(gs_serverIP, ZD_DEFAULT_SERVER_IP, sizeof(gs_serverIP) - 1);
    gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
    gs_serverPort = ZD_DEFAULT_PUSH_PORT;
    gs_pullPort = ZD_DEFAULT_PULL_PORT;
    return;
}

static RETURN_STATUS connConfigLoad(void)
{
    ConnConfig_t cfg;
    FsFile *f = fsOpenFile((char_t *)PROTO_METLLX_SERVER_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
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

    FsFile *f = fsOpenFile((char_t *)PROTO_METLLX_SERVER_FILE,
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

static uint16_t buildAckNack(uint8_t *buf, uint16_t sz, BOOL isAck)
{
    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, isAck ? FUNC_ACK : FUNC_NACK);
    metallixTLV_AddBool(&b, TAG_ACK_STATUS, isAck);
    return metallixTLV_PacketEnd(&b);
}

static uint16_t buildIdentMsg(uint8_t *buf, uint16_t sz)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, FUNC_IDENT);
    metallixTLV_AddBool(&b, TAG_REGISTERED, gs_registered);
    metallixTLV_AddString(&b, TAG_DEVICE_BRAND, PROTO_METLLX_BRAND);
    metallixTLV_AddString(&b, TAG_DEVICE_MODEL, PROTO_METLLX_MODEL);
    metallixTLV_AddString(&b, TAG_DEVICE_DATE, dateStr);
    metallixTLV_AddString(&b, TAG_PULL_IP, gs_deviceIP);
    metallixTLV_AddUint16(&b, TAG_PULL_PORT, (uint16_t)gs_pullPort);
    return metallixTLV_PacketEnd(&b);
}

static uint16_t buildAliveMsg(uint8_t *buf, uint16_t sz)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, FUNC_ALIVE);
    metallixTLV_AddString(&b, TAG_DEVICE_DATE, dateStr);
    return metallixTLV_PacketEnd(&b);
}

static uint16_t buildLogPacket(uint8_t *buf, uint16_t sz,
                                const char *logData,
                                int packetNum, BOOL stream)
{
    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, FUNC_LOG);
    metallixTLV_AddUint16(&b, TAG_PACKET_NUM, (uint16_t)packetNum);
    metallixTLV_AddBool(&b, TAG_PACKET_STREAM, stream);
    metallixTLV_AddString(&b, TAG_LOG_DATA, logData);
    return metallixTLV_PacketEnd(&b);
}

static uint16_t buildDataPacket(uint8_t *buf, uint16_t sz,
                                 MetallixTLV_Function_t func,
                                 const char *meterId,
                                 const char *data,
                                 int packetNum, BOOL stream)
{
    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, (uint8_t)func);
    metallixTLV_AddUint16(&b, TAG_PACKET_NUM, (uint16_t)packetNum);
    metallixTLV_AddBool(&b, TAG_PACKET_STREAM, stream);
    metallixTLV_AddString(&b, TAG_METER_ID, meterId);
    metallixTLV_AddString(&b, TAG_READOUT_DATA, data);
    return metallixTLV_PacketEnd(&b);
}

static uint16_t buildDirectivePacket(uint8_t *buf, uint16_t sz,
                                      const char *directive,
                                      int packetNum, BOOL stream)
{
    MetallixTLV_Builder_t b;
    metallixTLV_BuilderInit(&b, buf, sz);
    metallixTLV_PacketBegin(&b);
    metallixTLV_AddDeviceHeader(&b);
    metallixTLV_AddUint8(&b, TAG_FUNCTION, FUNC_DIRECTIVE_LIST);
    metallixTLV_AddUint16(&b, TAG_PACKET_NUM, (uint16_t)packetNum);
    metallixTLV_AddBool(&b, TAG_PACKET_STREAM, stream);
    metallixTLV_AddString(&b, TAG_DIRECTIVE_DATA, directive);
    return metallixTLV_PacketEnd(&b);
}

/* ================================================================== */
/*               Packetised send helpers                              */
/* ================================================================== */

static void sendLogPackets(void)
{
    char curBuf[PROTO_METLLX_DATA_CHUNK_SIZE + 1];
    char nxtBuf[PROTO_METLLX_DATA_CHUNK_SIZE + 1];

    S32 curLen = appLogRecRead(g_sysLoggerID, curBuf, PROTO_METLLX_DATA_CHUNK_SIZE);

    if (curLen <= 0)
    {
        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), "NO-LOG", 1, FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    int packetNum = 0;
    while (curLen > 0)
    {
        curBuf[curLen] = '\0';
        packetNum++;

        S32 nxtLen = appLogRecRead(g_sysLoggerID, nxtBuf, PROTO_METLLX_DATA_CHUNK_SIZE);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), curBuf, packetNum, hasMore);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

        if (!hasMore) break;

        memcpy(curBuf, nxtBuf, (size_t)nxtLen);
        curLen = nxtLen;
    }
}

static void sendFilePacketised(const char *channel,
                                MetallixTLV_Function_t func,
                                const char *meterId,
                                const char *filePath)
{
    FsFile *f = fsOpenFile((char_t *)filePath, FS_FILE_MODE_READ);
    if (f == NULL) return;

    char bufA[PROTO_METLLX_DATA_CHUNK_SIZE + 1];
    char bufB[PROTO_METLLX_DATA_CHUNK_SIZE + 1];
    char *cur = bufA;
    char *nxt = bufB;

    size_t curLen = 0;
    (void)fsReadFile(f, cur, PROTO_METLLX_DATA_CHUNK_SIZE, &curLen);
    if (curLen == 0) { fsCloseFile(f); return; }

    int packetNum = 0;
    while (curLen > 0)
    {
        cur[curLen] = '\0';
        packetNum++;

        size_t nxtLen = 0;
        (void)fsReadFile(f, nxt, PROTO_METLLX_DATA_CHUNK_SIZE, &nxtLen);
        BOOL hasMore = (nxtLen > 0);

        uint16_t sz = buildDataPacket(gs_txBuf, sizeof(gs_txBuf),
                                       func, meterId, cur,
                                       packetNum, hasMore);
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
                        char *host, size_t hostSz,
                        U16 *port,
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
            size_t uLen = (size_t)(colon - p);
            if (uLen >= userSz) uLen = userSz - 1;
            memcpy(user, p, uLen); user[uLen] = '\0';

            size_t pLen = (size_t)(at - colon - 1);
            if (pLen >= passSz) pLen = passSz - 1;
            memcpy(pass, colon + 1, pLen); pass[pLen] = '\0';
        }
        else
        {
            size_t uLen = (size_t)(at - p);
            if (uLen >= userSz) uLen = userSz - 1;
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

    const char *slash = strchr(p, '/');
    const char *hostEnd = slash ? slash : (p + strlen(p));
    const char *portColon = (const char *)memchr(p, ':', (size_t)(hostEnd - p));

    if (portColon != NULL)
    {
        size_t hLen = (size_t)(portColon - p);
        if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen); host[hLen] = '\0';
        *port = (U16)atoi(portColon + 1);
    }
    else
    {
        size_t hLen = (size_t)(hostEnd - p);
        if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen); host[hLen] = '\0';
        *port = FTP_DEFAULT_PORT;
    }

    if (slash != NULL) { strncpy(path, slash, pathSz - 1); path[pathSz - 1] = '\0'; }
    else               { path[0] = '/'; path[1] = '\0'; }

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
    if (outLen != NULL) *outLen = total;
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
            gs_pendingJobs[i].result = status;
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
            gs_pendingJobs[i].taskId       = taskId;
            gs_pendingJobs[i].completed    = FALSE;
            gs_pendingJobs[i].result       = EN_ERR_CODE_PENDING;
            gs_pendingJobs[i].isLoadProfile = isLoadProfile;
            return i;
        }
    }
    return -1;
}

/* ================================================================== */
/*               Pull-socket request handlers                        */
/* ================================================================== */

/* ---- Log ---- */
static void handleLogRequest(void)
{
    DEBUG_INFO("->[I] MetallixTLV: log request received");
    sendLogPackets();
}

/* ---- Setting ---- */
static void handleSettingRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: setting request received");
    BOOL success = TRUE;

    /* Server IP / Port update */
    char newIP[20] = {0};
    uint16_t newPort = 0;
    BOOL hasIP   = metallixTLV_GetString(parser, TAG_SERVER_IP, newIP, sizeof(newIP));
    BOOL hasPort = metallixTLV_GetUint16(parser, TAG_SERVER_PORT, &newPort);

    if (hasIP && newIP[0] != '\0' && hasPort && newPort > 0)
    {
        strncpy(gs_serverIP, newIP, sizeof(gs_serverIP) - 1);
        gs_serverPort = (int)newPort;
        serverConfigSave(gs_serverIP, gs_serverPort);

        appTcpConnManagerStop();
        zosDelayTask(500);
        appTcpConnManagerStart(gs_serverIP, gs_serverPort, gs_pullPort, appProtocolMetallixTLVPutIncomingMessage);
        appTcpConnManagerRequestConnect();

        DEBUG_INFO("->[I] MetallixTLV: server address updated to %s:%d", gs_serverIP, gs_serverPort);
        APP_LOG_REC(g_sysLoggerID, "Server address updated");
    }

    /* Meter operations — iterate TAG_METER_INDEX occurrences.
     * Each METER_INDEX marks the start of a new meter entry;
     * fields following it (until next METER_INDEX or packet end) belong to that meter. */
    metallixTLV_ParserReset(parser);

    uint16_t tag; const uint8_t *val; uint16_t vLen;
    BOOL inMeter = FALSE;

    char operation[16] = {0};
    MeterData_t md;

    while (metallixTLV_ParserNext(parser, &tag, &val, &vLen))
    {
        if (tag == TAG_METER_INDEX)
        {
            /* commit previous meter if any */
            if (inMeter)
            {
                if (0 == strcmp(operation, "add"))
                {
                    if (SUCCESS != appMeterOperationsAddMeter(&md))
                        success = FALSE;
                }
                else if (0 == strcmp(operation, "remove"))
                {
                    if (md.serialNumber[0] != '\0')
                    {
                        if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber))
                            success = FALSE;
                    }
                }
            }
            /* reset for new entry */
            inMeter = TRUE;
            memset(&md, 0, sizeof(md));
            memset(operation, 0, sizeof(operation));
            continue;
        }

        if (!inMeter) continue;

        switch (tag)
        {
            case TAG_METER_OPERATION:
            {
                uint16_t cp = vLen < sizeof(operation) - 1 ? vLen : (uint16_t)(sizeof(operation) - 1);
                memcpy(operation, val, cp); operation[cp] = '\0';
                break;
            }
            case TAG_METER_PROTOCOL:
            {
                uint16_t cp = vLen < sizeof(md.protocol) - 1 ? vLen : (uint16_t)(sizeof(md.protocol) - 1);
                memcpy(md.protocol, val, cp); md.protocol[cp] = '\0';
                break;
            }
            case TAG_METER_TYPE:
            {
                uint16_t cp = vLen < sizeof(md.type) - 1 ? vLen : (uint16_t)(sizeof(md.type) - 1);
                memcpy(md.type, val, cp); md.type[cp] = '\0';
                break;
            }
            case TAG_METER_BRAND:
            {
                uint16_t cp = vLen < sizeof(md.brand) - 1 ? vLen : (uint16_t)(sizeof(md.brand) - 1);
                memcpy(md.brand, val, cp); md.brand[cp] = '\0';
                break;
            }
            case TAG_METER_SERIAL_NUM:
            {
                uint16_t cp = vLen < sizeof(md.serialNumber) - 1 ? vLen : (uint16_t)(sizeof(md.serialNumber) - 1);
                memcpy(md.serialNumber, val, cp); md.serialNumber[cp] = '\0';
                break;
            }
            case TAG_METER_SERIAL_PORT:
            {
                uint16_t cp = vLen < sizeof(md.serialPort) - 1 ? vLen : (uint16_t)(sizeof(md.serialPort) - 1);
                memcpy(md.serialPort, val, cp); md.serialPort[cp] = '\0';
                break;
            }
            case TAG_METER_INIT_BAUD:
            {
                if (vLen >= 4)
                    md.initBaud = (int)(((uint32_t)val[0] << 24) | ((uint32_t)val[1] << 16) |
                                        ((uint32_t)val[2] << 8) | val[3]);
                break;
            }
            case TAG_METER_FIX_BAUD:
                md.fixBaud = (vLen >= 1 && val[0] != 0) ? TRUE : FALSE;
                break;
            case TAG_METER_FRAME:
            {
                uint16_t cp = vLen < sizeof(md.frame) - 1 ? vLen : (uint16_t)(sizeof(md.frame) - 1);
                memcpy(md.frame, val, cp); md.frame[cp] = '\0';
                break;
            }
            case TAG_METER_CUSTOMER_NUM:
            {
                uint16_t cp = vLen < sizeof(md.customerNumber) - 1 ? vLen : (uint16_t)(sizeof(md.customerNumber) - 1);
                memcpy(md.customerNumber, val, cp); md.customerNumber[cp] = '\0';
                break;
            }
            default:
                break;
        }
    }

    /* commit last meter entry */
    if (inMeter)
    {
        if (0 == strcmp(operation, "add"))
        {
            if (SUCCESS != appMeterOperationsAddMeter(&md))
                success = FALSE;
        }
        else if (0 == strcmp(operation, "remove"))
        {
            if (md.serialNumber[0] != '\0')
            {
                if (SUCCESS != appMeterOperationsDeleteMeter(md.serialNumber))
                    success = FALSE;
            }
        }
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), success);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
}

/* ---- FW Update ---- */
static void handleFwUpdateRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: fwUpdate request received");

    char address[192] = {0};
    if (!metallixTLV_GetString(parser, TAG_FW_ADDRESS, address, sizeof(address)) || address[0] == '\0')
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);

    char host[FTP_FIELD_MAX], user[FTP_FIELD_MAX], pass[FTP_FIELD_MAX], path[FTP_FIELD_MAX];
    U16 port = FTP_DEFAULT_PORT;

    if (parseFtpUrl(address, host, sizeof(host), &port,
                    user, sizeof(user), pass, sizeof(pass),
                    path, sizeof(path)))
    {
        AppSwUpdateInit(host, port, user, pass, path, FW_LOCAL_PATH);
        DEBUG_INFO("->[I] MetallixTLV: FW update started from %s", host);
        APP_LOG_REC(g_sysLoggerID, "FW update started");
    }
    else
    {
        DEBUG_ERROR("->[E] MetallixTLV: FTP URL parse failed");
        APP_LOG_REC(g_sysLoggerID, "FTP URL parse failed");
    }
}

/* ---- Readout ---- */
static void handleReadoutRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: readout request received");

    /* Rebuild a minimal JSON request string for AppMeterOperations,
       which expects JSON-formatted request data */
    char reqBuf[320];
    char directive[64] = {0};
    char meterSn[20] = {0};

    metallixTLV_GetString(parser, TAG_DIRECTIVE_NAME, directive, sizeof(directive));
    metallixTLV_GetString(parser, TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));

    snprintf(reqBuf, sizeof(reqBuf),
             "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\"}}",
             directive, meterSn);

    S32 tid = appMeterOperationsAddReadoutTask(reqBuf, meterTaskCallback);
    if (tid <= 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    if (pendingJobAllocSlot(tid, FALSE) < 0)
    {
        appMeterOperationsTaskIDFree(tid);
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
}

/* ---- Load Profile ---- */
static void handleLoadProfileRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: loadprofile request received");

    char directive[64] = {0};
    char meterSn[20]   = {0};
    char startDate[24] = {0};
    char endDate[24]   = {0};

    metallixTLV_GetString(parser, TAG_DIRECTIVE_NAME, directive, sizeof(directive));
    metallixTLV_GetString(parser, TAG_METER_SERIAL_NUM, meterSn, sizeof(meterSn));
    metallixTLV_GetString(parser, TAG_START_DATE, startDate, sizeof(startDate));
    metallixTLV_GetString(parser, TAG_END_DATE, endDate, sizeof(endDate));

    char reqBuf[320];
    snprintf(reqBuf, sizeof(reqBuf),
             "{\"directive\":\"%s\",\"METERSERIALNUMBER\":\"%s\","
             "\"startDate\":\"%s\",\"endDate\":\"%s\"}",
             directive, meterSn, startDate, endDate);

    S32 tid = appMeterOperationsAddProfileTask(reqBuf, meterTaskCallback);
    if (tid <= 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    if (pendingJobAllocSlot(tid, TRUE) < 0)
    {
        appMeterOperationsTaskIDFree(tid);
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
}

/* ---- Directive List ---- */
static void handleDirectiveListRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: directiveList request received");

    char filterId[64] = {0};
    metallixTLV_GetString(parser, TAG_DIRECTIVE_ID, filterId, sizeof(filterId));

    BOOL sendAll = (filterId[0] == '\0' || (filterId[0] == '*' && filterId[1] == '\0'));

    U32 count = appMeterOperationsGetDirectiveCount();
    if (count == 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    char dirBuf[PROTO_METLLX_PACKET_MAX_SIZE];
    int totalToSend = 0;

    if (sendAll) { totalToSend = (int)count; }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
            totalToSend = 1;
    }

    if (totalToSend == 0)
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    int sent = 0;
    int packetNum = 0;

    if (sendAll)
    {
        for (U32 i = 0; i < count; i++)
        {
            if (SUCCESS != appMeterOperationsGetDirectiveByIndex(i, dirBuf, sizeof(dirBuf)))
                continue;
            sent++;
            packetNum++;
            BOOL hasMore = (sent < totalToSend);
            uint16_t sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf, packetNum, hasMore);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
        {
            uint16_t sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), dirBuf, 1, FALSE);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        }
    }
}

/* ---- Directive Add ---- */
static void handleDirectiveAddRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: directiveAdd request received");

    char dirData[PROTO_METLLX_PACKET_MAX_SIZE] = {0};
    if (!metallixTLV_GetString(parser, TAG_DIRECTIVE_DATA, dirData, sizeof(dirData)) || dirData[0] == '\0')
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    S32 result = appMeterOperationsAddDirective(dirData);
    BOOL ok = (result >= 0);

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
}

/* ---- Directive Delete ---- */
static void handleDirectiveDeleteRequest(MetallixTLV_Parser_t *parser)
{
    DEBUG_INFO("->[I] MetallixTLV: directiveDelete request received");

    char filterId[64] = {0};
    if (!metallixTLV_GetString(parser, TAG_DIRECTIVE_ID, filterId, sizeof(filterId)) || filterId[0] == '\0')
    {
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    BOOL ok = (SUCCESS == appMeterOperationsDeleteDirective(filterId));

    uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
}

/* ================================================================== */
/*           Pull message dispatcher                                 */
/* ================================================================== */

static void processPullMessage(const uint8_t *raw, uint16_t rawLen)
{
    MetallixTLV_Parser_t parser;
    if (!metallixTLV_ParserInit(&parser, raw, rawLen))
    {
        DEBUG_WARNING("->[W] MetallixTLV: invalid packet on pull");
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    MetallixTLV_Function_t func;
    if (!metallixTLV_GetFunction(&parser, &func))
    {
        DEBUG_WARNING("->[W] MetallixTLV: no FUNCTION tag in pull packet");
        uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
        return;
    }

    switch (func)
    {
        case FUNC_LOG:            handleLogRequest();                     break;
        case FUNC_SETTING:        handleSettingRequest(&parser);          break;
        case FUNC_FW_UPDATE:      handleFwUpdateRequest(&parser);        break;
        case FUNC_READOUT:        handleReadoutRequest(&parser);         break;
        case FUNC_LOADPROFILE:    handleLoadProfileRequest(&parser);     break;
        case FUNC_DIRECTIVE_LIST: handleDirectiveListRequest(&parser);   break;
        case FUNC_DIRECTIVE_ADD:  handleDirectiveAddRequest(&parser);    break;
        case FUNC_DIRECTIVE_DEL:  handleDirectiveDeleteRequest(&parser); break;
        default:
            DEBUG_WARNING("->[W] MetallixTLV: unknown pull function 0x%02X", (unsigned)func);
            {
                uint16_t sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
                appTcpConnManagerSend(PULL_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            }
            break;
    }
}

/* ================================================================== */
/*              Pending meter-job processing                         */
/* ================================================================== */

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

static void deliverMeterData(int jobIdx)
{
    PendingJob_t *job = &gs_pendingJobs[jobIdx];
    MetallixTLV_Function_t func = job->isLoadProfile ? FUNC_LOADPROFILE : FUNC_READOUT;

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
        while ((osGetSystemTime() - t0) < PROTO_METLLX_CONNECT_TIMEOUT_MS)
        {
            if (appTcpConnManagerIsConnectedPush()) break;
            zosDelayTask(100);
        }
        if (!appTcpConnManagerIsConnectedPush())
        {
            DEBUG_ERROR("->[E] MetallixTLV: push connect timeout, data delivery skipped");
            return;
        }
    }

    sendFilePacketised(PUSH_TCP_SOCK_NAME, func, meterId, dataPath);

    gs_pushRx.ready = FALSE;
    U32 t0 = osGetSystemTime();
    while ((osGetSystemTime() - t0) < PROTO_METLLX_ACK_TIMEOUT_MS)
    {
        if (gs_pushRx.ready) { gs_pushRx.ready = FALSE; break; }
        zosDelayTask(100);
    }
}

static void processPendingMeterJobs(void)
{
    for (int i = 0; i < MAX_PENDING_JOBS; i++)
    {
        if (gs_pendingJobs[i].taskId == 0)   continue;
        if (!gs_pendingJobs[i].completed)     continue;

        if (gs_pendingJobs[i].result == EN_ERR_CODE_SUCCESS)
        {
            deliverMeterData(i);
        }
        else
        {
            DEBUG_WARNING("->[W] MetallixTLV: meter task %d failed (err %d)",
                          gs_pendingJobs[i].taskId, gs_pendingJobs[i].result);
        }

        cleanupMeterFiles(gs_pendingJobs[i].taskId);
        appMeterOperationsTaskIDFree(gs_pendingJobs[i].taskId);
        gs_pendingJobs[i].taskId    = 0;
        gs_pendingJobs[i].completed = FALSE;
    }
}

/* ================================================================== */
/*                     Protocol task                                  */
/* ================================================================== */

static void protocolTaskFunc(void *arg)
{
    (void)arg;
    DEBUG_INFO("->[I] %s task started", METALLIX_TASK_NAME);
    appTskMngImOK(gs_taskId);

    /* --- Start TCP --- */
    if (0 != appTcpConnManagerStart(gs_serverIP, gs_serverPort,
                                     gs_pullPort, appProtocolMetallixTLVPutIncomingMessage))
    {
        DEBUG_ERROR("->[E] MetallixTLV: TCP conn manager start failed");
        APP_LOG_REC(g_sysLoggerID, "MetallixTLV: TCP start fail");
        gs_running = FALSE;
        return;
    }
    appTcpConnManagerRequestConnect();
    zosDelayTask(500);

    /* --- Registration phase --- */
    gs_registered = registerStateLoad();

    if (!gs_registered)
    {
        DEBUG_INFO("->[I] MetallixTLV: device not registered, starting ident");

        while (gs_running && !gs_registered)
        {
            appTcpConnManagerRequestPushConnect();

            U32 t0 = osGetSystemTime();
            while ((osGetSystemTime() - t0) < PROTO_METLLX_CONNECT_TIMEOUT_MS && gs_running)
            {
                if (appTcpConnManagerIsConnectedPush()) break;
                zosDelayTask(100);
                appTskMngImOK(gs_taskId);
            }

            if (!appTcpConnManagerIsConnectedPush())
            {
                DEBUG_WARNING("->[W] MetallixTLV: push connect timeout, retrying in %d s", PROTO_METLLX_REGISTER_RETRY_S);
                for (int w = 0; w < PROTO_METLLX_REGISTER_RETRY_S * 10 && gs_running; w++)
                {
                    zosDelayTask(100);
                    appTskMngImOK(gs_taskId);
                }
                continue;
            }

            /* Send ident */
            uint16_t sz = buildIdentMsg(gs_txBuf, sizeof(gs_txBuf));
            appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
            DEBUG_DEBUG("->[D] MetallixTLV: ident sent (%u bytes)", (unsigned)sz);

            /* Wait for server response */
            gs_pushRx.ready = FALSE;
            t0 = osGetSystemTime();
            while ((osGetSystemTime() - t0) < (U32)PROTO_METLLX_REGISTER_RETRY_S * 1000U && gs_running)
            {
                if (gs_pushRx.ready)
                {
                    MetallixTLV_Parser_t rp;
                    if (metallixTLV_ParserInit(&rp, gs_pushRx.data, (uint16_t)gs_pushRx.length))
                    {
                        MetallixTLV_Function_t rFunc;
                        if (metallixTLV_GetFunction(&rp, &rFunc) && rFunc == FUNC_IDENT)
                        {
                            BOOL regVal = FALSE;
                            if (metallixTLV_GetBool(&rp, TAG_REGISTER, &regVal) && regVal)
                            {
                                gs_registered = TRUE;
                                registerStateSave(TRUE);
                                DEBUG_INFO("->[I] MetallixTLV: device registered successfully");
                                APP_LOG_REC(g_sysLoggerID, "Device registered");
                            }
                        }
                    }
                    gs_pushRx.ready = FALSE;
                    break;
                }
                zosDelayTask(100);
                appTskMngImOK(gs_taskId);
            }

            if (!gs_registered)
            {
                appTcpConnManagerRequestPushDisconnect();
                DEBUG_DEBUG("->[D] MetallixTLV: registration attempt failed, retrying");
                for (int w = 0; w < PROTO_METLLX_REGISTER_RETRY_S * 10 && gs_running; w++)
                {
                    zosDelayTask(100);
                    appTskMngImOK(gs_taskId);
                }
            }
        }
    }
    else
    {
        DEBUG_INFO("->[I] MetallixTLV: device already registered");
    }

    /* --- Main loop (registered) --- */
    U32 aliveTimer = osGetSystemTime();

    while (gs_running)
    {
        appTskMngImOK(gs_taskId);

        if (!appTcpConnManagerIsConnectedPush())
            appTcpConnManagerRequestPushConnect();

        /* ---- Alive ---- */
        U32 now = osGetSystemTime();
        if ((now - aliveTimer) >= (U32)PROTO_METLLX_ALIVE_INTERVAL_S * 1000U)
        {
            if (appTcpConnManagerIsConnectedPush())
            {
                uint16_t sz = buildAliveMsg(gs_txBuf, sizeof(gs_txBuf));
                appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, (char *)gs_txBuf, (unsigned int)sz);
                DEBUG_DEBUG("->[D] MetallixTLV: alive sent");
            }
            aliveTimer = now;
        }

        /* ---- Pull requests ---- */
        if (gs_pullRx.ready)
        {
            processPullMessage(gs_pullRx.data, (uint16_t)gs_pullRx.length);
            gs_pullRx.ready = FALSE;
        }

        /* ---- Push responses ---- */
        if (gs_pushRx.ready)
            gs_pushRx.ready = FALSE;

        /* ---- Completed meter tasks ---- */
        processPendingMeterJobs();

        zosDelayTask(100);
    }

    appTcpConnManagerStop();
    DEBUG_INFO("->[I] %s task stopped", METALLIX_TASK_NAME);
}

/* ================================================================== */
/*                       PUBLIC FUNCTIONS                             */
/* ================================================================== */

RETURN_STATUS appProtocolMetallixTLVInit(const char *serialNumber)
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

    if (!serverConfigLoad(gs_serverIP, sizeof(gs_serverIP), &gs_serverPort))
    {
        strncpy(gs_serverIP, serverIP, sizeof(gs_serverIP) - 1);
        gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
        gs_serverPort = serverPort;
    }

    memset(&gs_pushRx, 0, sizeof(gs_pushRx));
    memset(&gs_pullRx, 0, sizeof(gs_pullRx));
    memset(gs_pendingJobs, 0, sizeof(gs_pendingJobs));

    gs_registered = FALSE;
    gs_running    = FALSE;
    gs_taskId     = OS_INVALID_TASK_ID;

    return SUCCESS;
}

RETURN_STATUS appProtocolMetallixTLVStart(void)
{
    if (gs_running) return SUCCESS;

    gs_running = TRUE;

    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = METALLIX_TASK_STACK_SIZE;

    gs_taskId = appTskMngCreate(METALLIX_TASK_NAME, protocolTaskFunc, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_taskId)
    {
        gs_running = FALSE;
        DEBUG_ERROR("->[E] MetallixTLV: task creation failed");
        APP_LOG_REC(g_sysLoggerID, "MetallixTLV task creation failed");
        return FAILURE;
    }

    DEBUG_INFO("->[I] Protocol MetallixTLV started");
    APP_LOG_REC(g_sysLoggerID, "Protocol MetallixTLV started");
    return SUCCESS;
}

RETURN_STATUS appProtocolMetallixTLVStop(void)
{
    if (!gs_running) return SUCCESS;

    gs_running = FALSE;
    zosDelayTask(500);

    if (OS_INVALID_TASK_ID != gs_taskId)
    {
        appTskMngDelete(gs_taskId);
        gs_taskId = OS_INVALID_TASK_ID;
    }

    DEBUG_INFO("->[I] Protocol MetallixTLV stopped");
    APP_LOG_REC(g_sysLoggerID, "Protocol MetallixTLV stopped");
    return SUCCESS;
}

void appProtocolMetallixTLVPutIncomingMessage(const char *channel,
                                     const char *data,
                                     unsigned int dataLength)
{
    if (channel == NULL || data == NULL || dataLength == 0)
        return;

    if (dataLength > PROTO_METLLX_PACKET_MAX_SIZE)
        dataLength = PROTO_METLLX_PACKET_MAX_SIZE;

    if (0 == strcmp(channel, PUSH_TCP_SOCK_NAME))
    {
        memcpy(gs_pushRx.data, data, dataLength);
        gs_pushRx.length = dataLength;
        gs_pushRx.ready  = TRUE;
    }
    else if (0 == strcmp(channel, PULL_TCP_SOCK_NAME))
    {
        memcpy(gs_pullRx.data, data, dataLength);
        gs_pullRx.length = dataLength;
        gs_pullRx.ready  = TRUE;
    }
}

/******************************** End Of File *********************************/
