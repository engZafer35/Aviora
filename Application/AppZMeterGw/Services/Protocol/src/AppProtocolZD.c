/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Mar 28, 2026
* #File Name    : AppProtocolZD.c
*******************************************************************************/
/******************************************************************************
* JSON-over-TCP protocol handler for IoT meter gateway.
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
#include "AppProtocolZD.h"
#include "AppTcpConnManager.h"
#include "AppMeterOperations.h"
#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppTimeService.h"
#include "AppSWUpdate.h"
#include "ZDJson.h"
#include "fs_port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/****************************** MACRO DEFINITIONS *****************************/
#define PROTO_TASK_STACK_SIZE  (4096U)
#define PROTO_TASK_NAME       "ProtoZD"

#define MAX_PENDING_JOBS      (4)
#define MAX_PATH_LEN          (96)

#define FTP_FIELD_MAX         (64)
#define FTP_DEFAULT_PORT      (21)
#define FW_LOCAL_PATH         "/tmp/firmware.bin"

/******************************* TYPE DEFINITIONS *****************************/

typedef struct
{
    char data[PROTOCOL_PACKET_MAX_SIZE + 1];
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

static char gs_txBuf[PROTOCOL_PACKET_MAX_SIZE];

/***************************** STATIC FUNCTIONS  ******************************/

/* ------------------------------------------------------------------ */
/*                     Register-state persistence                     */
/* ------------------------------------------------------------------ */

static BOOL registerStateLoad(void)
{
    FsFile *f = fsOpenFile((char_t *)PROTOCOL_REGISTER_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FALSE;

    U8 val = 0;
    size_t got = 0;
    (void)fsReadFile(f, &val, 1, &got);
    fsCloseFile(f);

    return (got == 1 && val == 1) ? TRUE : FALSE;
}

static void registerStateSave(BOOL reg)
{
    FsFile *f = fsOpenFile((char_t *)PROTOCOL_REGISTER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return;

    U8 val = reg ? 1 : 0;
    (void)fsWriteFile(f, &val, 1);
    fsCloseFile(f);
}

/* ------------------------------------------------------------------ */
/*                   Server-address persistence                       */
/* ------------------------------------------------------------------ */
static void connConfigDefault(void)
{
    strncpy(gs_serverIP, ZD_DEFAULT_SERVER_IP, sizeof(gs_serverIP) - 1);
    gs_serverIP[sizeof(gs_serverIP) - 1] = '\0';
    gs_serverPort = ZD_DEFAULT_PUSH_PORT;
    gs_pullPort = ZD_DEFAULT_PULL_PORT;    
}

static RETURN_STATUS connConfigLoad(void)
{
    ConnConfig_t cfg;
    FsFile *f = fsOpenFile((char_t *)PROTOCOL_CONN_CONFIG_FILE, FS_FILE_MODE_READ);
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
    ConnConfig_t cfg;
    RETURN_STATUS retVal = FAILURE;

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ip, gs_serverIP, sizeof(cfg.ip) - 1);
    cfg.port = gs_serverPort;
    cfg.pullPort = gs_pullPort;

    FsFile *f = fsOpenFile((char_t *)PROTOCOL_CONN_CONFIG_FILE,
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

/* ------------------------------------------------------------------ */
/*                          Time helper                               */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/*                       JSON builder helpers                         */
/* ------------------------------------------------------------------ */

static void jsonAddDeviceHeader(ZDJson_Builder_t *jb)
{
    ZDJson_OpenObject(jb, "device");
    ZDJson_AddString(jb, "flag", PROTOCOL_FLAG);
    ZDJson_AddString(jb, "serialNumber", gs_serialNumber);
    ZDJson_CloseObject(jb);
}

static int buildAckNack(char *buf, size_t sz, BOOL isAck)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    jsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", isAck ? "ack" : "nack");
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int buildIdentMsg(char *buf, size_t sz)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    jsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "ident");
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddBool(&jb, "registered", gs_registered ? true : false);
    ZDJson_AddString(&jb, "brand", PROTOCOL_BRAND);
    ZDJson_AddString(&jb, "model", PROTOCOL_MODEL);
    ZDJson_AddString(&jb, "deviceDate", dateStr);
    ZDJson_AddString(&jb, "pullIP", gs_deviceIP);
    ZDJson_AddNumber(&jb, "pullPort", gs_pullPort);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int buildAliveMsg(char *buf, size_t sz)
{
    char dateStr[24];
    getDeviceDateStr(dateStr, sizeof(dateStr));

    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    jsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "alive");
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddString(&jb, "deviceDate", dateStr);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int buildLogPacket(char *buf, size_t sz,
                          const char *logData, int packetNum, BOOL stream)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    jsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "log");
    ZDJson_AddNumber(&jb, "packetNum", packetNum);
    ZDJson_AddBool(&jb, "packetStream", stream ? true : false);
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddString(&jb, "log", logData);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

/**
 * Build readout / loadprofile data packet.
 * @param funcName  "readout" or "loadprofile"
 * @param meterId   meter identification string (or "load-profile-no-id")
 * @param data      chunk of payload data
 */
static int buildDataPacket(char *buf, size_t sz,
                           const char *funcName,
                           const char *meterId,
                           const char *data,
                           int packetNum, BOOL stream)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    jsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", funcName);
    ZDJson_AddNumber(&jb, "packetNum", packetNum);
    ZDJson_AddBool(&jb, "packetStream", stream ? true : false);
    ZDJson_OpenObject(&jb, "response");
    ZDJson_OpenObject(&jb, "data");
    ZDJson_AddString(&jb, "id", meterId);
    ZDJson_AddString(&jb, "readout", data);
    ZDJson_CloseObject(&jb); /* data   */
    ZDJson_CloseObject(&jb); /* response */
    ZDJson_CloseObject(&jb); /* root   */
    return (int)jb.pos;
}

/**
 * Build directiveList response packet.
 * Uses snprintf because the directive value is already valid JSON.
 */
static int buildDirectivePacket(char *buf, size_t sz,
                                const char *directive,
                                int packetNum, BOOL stream)
{
    return snprintf(buf, sz,
        "{\"device\":{\"flag\":\"%s\",\"serialNumber\":\"%s\"},"
        "\"function\":\"directiveList\","
        "\"packetNum\":%d,"
        "\"packetStream\":%s,"
        "\"response\":{\"directive\":%s}}",
        PROTOCOL_FLAG, gs_serialNumber,
        packetNum,
        stream ? "true" : "false",
        directive);
}

/* ------------------------------------------------------------------ */
/*               Packetised send helpers                              */
/* ------------------------------------------------------------------ */

/**
 * Read log data from system logger and send as 1024-byte JSON packets.
 * Uses look-ahead to determine packetStream correctly.
 */
static void sendLogPackets(void)
{
    char curBuf[PROTOCOL_DATA_CHUNK_SIZE + 1];
    char nxtBuf[PROTOCOL_DATA_CHUNK_SIZE + 1];

    S32 curLen = appLogRecRead(g_sysLoggerID, curBuf, PROTOCOL_DATA_CHUNK_SIZE);

    if (curLen <= 0)
    {
        int sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), "NO-LOG", 1, FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    int packetNum = 0;

    while (curLen > 0)
    {
        curBuf[curLen] = '\0';
        packetNum++;

        S32 nxtLen = appLogRecRead(g_sysLoggerID, nxtBuf, PROTOCOL_DATA_CHUNK_SIZE);
        BOOL hasMore = (nxtLen > 0);

        int sz = buildLogPacket(gs_txBuf, sizeof(gs_txBuf), curBuf, packetNum, hasMore);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);

        if (!hasMore)
            break;

        memcpy(curBuf, nxtBuf, (size_t)nxtLen);
        curLen = nxtLen;
    }
}

/**
 * Read a file and send its content via the given channel using
 * readout / loadprofile JSON framing.
 */
static void sendFilePacketised(const char *channel,
                               const char *funcName,
                               const char *meterId,
                               const char *filePath)
{
    FsFile *f = fsOpenFile((char_t *)filePath, FS_FILE_MODE_READ);
    if (f == NULL)
        return;

    char bufA[PROTOCOL_DATA_CHUNK_SIZE + 1];
    char bufB[PROTOCOL_DATA_CHUNK_SIZE + 1];
    char *cur = bufA;
    char *nxt = bufB;

    size_t curLen = 0;
    (void)fsReadFile(f, cur, PROTOCOL_DATA_CHUNK_SIZE, &curLen);

    if (curLen == 0)
    {
        fsCloseFile(f);
        return;
    }

    int packetNum = 0;

    while (curLen > 0)
    {
        cur[curLen] = '\0';
        packetNum++;

        size_t nxtLen = 0;
        (void)fsReadFile(f, nxt, PROTOCOL_DATA_CHUNK_SIZE, &nxtLen);
        BOOL hasMore = (nxtLen > 0);

        int sz = buildDataPacket(gs_txBuf, sizeof(gs_txBuf),
                                 funcName, meterId, cur,
                                 packetNum, hasMore);
        appTcpConnManagerSend(channel, gs_txBuf, (unsigned int)sz);

        if (!hasMore)
            break;

        /* swap pointers */
        char *tmp = cur;
        cur = nxt;
        nxt = tmp;
        curLen = nxtLen;
    }

    fsCloseFile(f);
}

/* ------------------------------------------------------------------ */
/*                      FTP URL parser                                */
/* ------------------------------------------------------------------ */

static BOOL parseFtpUrl(const char *url,
                        char *host, size_t hostSz,
                        U16 *port,
                        char *user, size_t userSz,
                        char *pass, size_t passSz,
                        char *path, size_t pathSz)
{
    if (strncmp(url, "ftp://", 6) != 0)
        return FALSE;

    const char *p = url + 6;

    /* user[:password]@ */
    const char *at = strchr(p, '@');
    if (at != NULL)
    {
        const char *colon = (const char *)memchr(p, ':', (size_t)(at - p));
        if (colon != NULL)
        {
            size_t uLen = (size_t)(colon - p);
            if (uLen >= userSz) uLen = userSz - 1;
            memcpy(user, p, uLen);
            user[uLen] = '\0';

            size_t pLen = (size_t)(at - colon - 1);
            if (pLen >= passSz) pLen = passSz - 1;
            memcpy(pass, colon + 1, pLen);
            pass[pLen] = '\0';
        }
        else
        {
            size_t uLen = (size_t)(at - p);
            if (uLen >= userSz) uLen = userSz - 1;
            memcpy(user, p, uLen);
            user[uLen] = '\0';
            pass[0] = '\0';
        }
        p = at + 1;
    }
    else
    {
        strncpy(user, "anonymous", userSz - 1);
        user[userSz - 1] = '\0';
        pass[0] = '\0';
    }

    /* host[:port] */
    const char *slash = strchr(p, '/');
    const char *hostEnd = slash ? slash : (p + strlen(p));
    const char *portColon = (const char *)memchr(p, ':', (size_t)(hostEnd - p));

    if (portColon != NULL)
    {
        size_t hLen = (size_t)(portColon - p);
        if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen);
        host[hLen] = '\0';
        *port = (U16)atoi(portColon + 1);
    }
    else
    {
        size_t hLen = (size_t)(hostEnd - p);
        if (hLen >= hostSz) hLen = hostSz - 1;
        memcpy(host, p, hLen);
        host[hLen] = '\0';
        *port = FTP_DEFAULT_PORT;
    }

    /* path */
    if (slash != NULL)
    {
        strncpy(path, slash, pathSz - 1);
        path[pathSz - 1] = '\0';
    }
    else
    {
        path[0] = '/';
        path[1] = '\0';
    }

    return TRUE;
}

/* ------------------------------------------------------------------ */
/*                 FS helper  (read whole text file)                  */
/* ------------------------------------------------------------------ */

static RETURN_STATUS readWholeText(const char *path, char *buf, size_t cap, size_t *outLen)
{
    FsFile *f = fsOpenFile((char_t *)path, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    size_t total = 0;
    for (;;)
    {
        if (total + 1 >= cap)
            break;
        size_t chunk = 0;
        error_t e = fsReadFile(f, buf + total, cap - 1U - total, &chunk);
        if (e == ERROR_END_OF_FILE || chunk == 0)
            break;
        total += chunk;
    }
    buf[total] = '\0';
    if (outLen != NULL)
        *outLen = total;
    fsCloseFile(f);
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*                    Meter-task callback                             */
/* ------------------------------------------------------------------ */

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
            gs_pendingJobs[i].taskId = taskId;
            gs_pendingJobs[i].completed = FALSE;
            gs_pendingJobs[i].result = EN_ERR_CODE_PENDING;
            gs_pendingJobs[i].isLoadProfile = isLoadProfile;
            return i;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/*               Pull-socket request handlers                        */
/* ------------------------------------------------------------------ */

/* ---- 5. Log ---- */
static void handleLogRequest(void)
{
    DEBUG_INFO("->[I] Proto: log request received");
    sendLogPackets();
}

/* ---- 6. Setting ---- */
static void handleSettingRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: setting request received");

    BOOL success = TRUE;

    char reqObj[512];
    if (FALSE == ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    /* --- Server IP / Port update --- */
    char serverObj[128];
    if (ZDJson_GetObjectValue(reqObj, "Server", serverObj, sizeof(serverObj)))
    {
        char newIP[20] = {0};
        int newPort = 0;

        ZDJson_GetStringValue(serverObj, "ip", newIP, sizeof(newIP));
        ZDJson_GetNumberValue(serverObj, "port", &newPort);

        if (newIP[0] != '\0' && newPort > 0)
        {
            strncpy(gs_serverIP, newIP, sizeof(gs_serverIP) - 1);
            gs_serverPort = newPort;
            serverConfigSave();

            appTcpConnManagerStop();
            zosDelayTask(500);
            appTcpConnManagerStart(gs_serverIP, gs_serverPort, gs_pullPort, appProtocolZDPutIncomingMessage);
            appTcpConnManagerRequestConnect();

            DEBUG_INFO("->[I] Proto: server address updated to %s:%d", gs_serverIP, gs_serverPort);
            APP_LOG_REC(g_sysLoggerID, "Server address updated");
        }
    }

    /* --- Meter operations (add / remove) --- */
    int idx = 0;
    char meterEntry[384];
    while (ZDJson_GetArrayObject(reqObj, "meters", idx, meterEntry, sizeof(meterEntry)))
    {
        char operation[16] = {0};
        ZDJson_GetStringValue(meterEntry, "operation", operation, sizeof(operation));

        char meterObj[320];
        if (FALSE == ZDJson_GetObjectValue(meterEntry, "meter", meterObj, sizeof(meterObj)))
        {
            idx++;
            continue;
        }

        if (0 == strcmp(operation, "add"))
        {
            MeterData_t md;
            memset(&md, 0, sizeof(md));
            ZDJson_GetStringValue(meterObj, "protocol",     md.protocol,     sizeof(md.protocol));
            ZDJson_GetStringValue(meterObj, "type",          md.type,         sizeof(md.type));
            ZDJson_GetStringValue(meterObj, "brand",         md.brand,        sizeof(md.brand));
            ZDJson_GetStringValue(meterObj, "serialNumber",  md.serialNumber, sizeof(md.serialNumber));
            ZDJson_GetStringValue(meterObj, "serialPort",    md.serialPort,   sizeof(md.serialPort));
            ZDJson_GetStringValue(meterObj, "frame",         md.frame,        sizeof(md.frame));
            ZDJson_GetNumberValue(meterObj, "initBaud",      &md.initBaud);

            bool fb = false;
            ZDJson_GetBoolValue(meterObj, "fixBaud", &fb);
            md.fixBaud = fb ? TRUE : FALSE;

            if (SUCCESS != appMeterOperationsAddMeter(&md))
                success = FALSE;
        }
        else if (0 == strcmp(operation, "remove"))
        {
            char sn[20] = {0};
            ZDJson_GetStringValue(meterObj, "serialNumber", sn, sizeof(sn));
            if (sn[0] != '\0')
            {
                if (SUCCESS != appMeterOperationsDeleteMeter(sn))
                    success = FALSE;
            }
        }

        idx++;
    }

    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), success);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
}

/* ---- 7. Firmware update ---- */
static void handleFwUpdateRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: fwUpdate request received");

    char reqObj[256];
    if (FALSE == ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    char address[192] = {0};
    ZDJson_GetStringValue(reqObj, "address", address, sizeof(address));

    if (address[0] == '\0')
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    /* ACK first, then start FTP */
    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);

    char host[FTP_FIELD_MAX], user[FTP_FIELD_MAX], pass[FTP_FIELD_MAX], path[FTP_FIELD_MAX];
    U16 port = FTP_DEFAULT_PORT;

    if (parseFtpUrl(address, host, sizeof(host), &port,
                    user, sizeof(user), pass, sizeof(pass),
                    path, sizeof(path)))
    {
        AppSwUpdateInit(host, port, user, pass, path, FW_LOCAL_PATH);
        DEBUG_INFO("->[I] Proto: FW update started from %s", host);
        APP_LOG_REC(g_sysLoggerID, "FW update started");
    }
    else
    {
        DEBUG_ERROR("->[E] Proto: FTP URL parse failed");
        APP_LOG_REC(g_sysLoggerID, "FTP URL parse failed");
    }
}

/* ---- 8. Readout ---- */
static void handleReadoutRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: readout request received");

    char reqObj[320];
    if (FALSE == ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    if (FALSE == ZDJson_IsValid(reqObj))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    S32 tid = appMeterOperationsAddReadoutTask(reqObj, meterTaskCallback);
    if (tid <= 0)
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    if (pendingJobAllocSlot(tid, FALSE) < 0)
    {
        appMeterOperationsTaskIDFree(tid);
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
}

/* ---- 9. Load profile ---- */
static void handleLoadProfileRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: loadprofile request received");

    char reqObj[320];
    if (FALSE == ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    if (FALSE == ZDJson_IsValid(reqObj))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    S32 tid = appMeterOperationsAddProfileTask(reqObj, meterTaskCallback);
    if (tid <= 0)
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    if (pendingJobAllocSlot(tid, TRUE) < 0)
    {
        appMeterOperationsTaskIDFree(tid);
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
}

/* ---- 10. Directive list ---- */
static void handleDirectiveListRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: directiveList request received");

    char reqObj[128];
    char filterId[MAX_KEY_LENGTH] = {0};

    if (ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        char filterObj[128];
        if (ZDJson_GetObjectValue(reqObj, "filter", filterObj, sizeof(filterObj)))
        {
            ZDJson_GetStringValue(filterObj, "id", filterId, sizeof(filterId));
        }
    }

    BOOL sendAll = (filterId[0] == '\0' || (filterId[0] == '*' && filterId[1] == '\0'));

    U32 count = appMeterOperationsGetDirectiveCount();
    if (count == 0)
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    char dirBuf[PROTOCOL_PACKET_MAX_SIZE];
    int packetNum = 0;
    int totalToSend = 0;

    /* first pass: count matching directives */
    if (sendAll)
    {
        totalToSend = (int)count;
    }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
            totalToSend = 1;
    }

    if (totalToSend == 0)
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), TRUE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    int sent = 0;

    if (sendAll)
    {
        for (U32 i = 0; i < count; i++)
        {
            if (SUCCESS != appMeterOperationsGetDirectiveByIndex(i, dirBuf, sizeof(dirBuf)))
                continue;

            sent++;
            packetNum++;
            BOOL hasMore = (sent < totalToSend);

            int sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf),
                                          dirBuf, packetNum, hasMore);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        }
    }
    else
    {
        if (SUCCESS == appMeterOperationsGetDirective(filterId, dirBuf, sizeof(dirBuf)))
        {
            int sz = buildDirectivePacket(gs_txBuf, sizeof(gs_txBuf),
                                          dirBuf, 1, FALSE);
            appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        }
    }
}

/* ---- 11. Directive add ---- */
static void handleDirectiveAddRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: directiveAdd request received");

    char reqObj[PROTOCOL_PACKET_MAX_SIZE];
    if (FALSE == ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    char dirObj[PROTOCOL_PACKET_MAX_SIZE];
    if (FALSE == ZDJson_GetObjectValue(reqObj, "directive", dirObj, sizeof(dirObj)))
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    S32 result = appMeterOperationsAddDirective(dirObj);
    BOOL ok = (result >= 0);

    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
}

/* ---- 12. Directive delete ---- */
static void handleDirectiveDeleteRequest(const char *json)
{
    DEBUG_INFO("->[I] Proto: directiveDelete request received");

    char reqObj[128];
    char filterId[MAX_KEY_LENGTH] = {0};

    if (ZDJson_GetObjectValue(json, "request", reqObj, sizeof(reqObj)))
    {
        char filterObj[128];
        if (ZDJson_GetObjectValue(reqObj, "filter", filterObj, sizeof(filterObj)))
        {
            ZDJson_GetStringValue(filterObj, "id", filterId, sizeof(filterId));
        }
    }

    if (filterId[0] == '\0')
    {
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    BOOL ok = (SUCCESS == appMeterOperationsDeleteDirective(filterId));

    int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
}

/* ------------------------------------------------------------------ */
/*           Pull message dispatcher                                 */
/* ------------------------------------------------------------------ */

static void processPullMessage(const char *json)
{
    if (FALSE == ZDJson_IsValid(json))
    {
        DEBUG_WARNING("->[W] Proto: invalid JSON on pull");
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
        return;
    }

    char funcStr[24] = {0};
    ZDJson_GetStringValue(json, "function", funcStr, sizeof(funcStr));

    if      (0 == strcmp(funcStr, "log"))             handleLogRequest();
    else if (0 == strcmp(funcStr, "setting"))         handleSettingRequest(json);
    else if (0 == strcmp(funcStr, "fwUpdate"))        handleFwUpdateRequest(json);
    else if (0 == strcmp(funcStr, "readout"))         handleReadoutRequest(json);
    else if (0 == strcmp(funcStr, "loadprofile"))     handleLoadProfileRequest(json);
    else if (0 == strcmp(funcStr, "directiveList"))   handleDirectiveListRequest(json);
    else if (0 == strcmp(funcStr, "directiveAdd"))    handleDirectiveAddRequest(json);
    else if (0 == strcmp(funcStr, "directiveDelete")) handleDirectiveDeleteRequest(json);
    else
    {
        DEBUG_WARNING("->[W] Proto: unknown pull function '%s'", funcStr);
        int sz = buildAckNack(gs_txBuf, sizeof(gs_txBuf), FALSE);
        appTcpConnManagerSend(PULL_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
    }
}

/* ------------------------------------------------------------------ */
/*              Pending meter-job processing                         */
/* ------------------------------------------------------------------ */

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
    const char *funcName = job->isLoadProfile ? "loadprofile" : "readout";

    /* Read meter ID */
    char meterId[128] = {0};
    char idPath[MAX_PATH_LEN];
    snprintf(idPath, sizeof(idPath), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);

    size_t got = 0;
    if (SUCCESS != readWholeText(idPath, meterId, sizeof(meterId), &got) || got == 0)
        strncpy(meterId, "unknown", sizeof(meterId));

    if (job->isLoadProfile)
        strncpy(meterId, "load-profile-no-id", sizeof(meterId));

    /* Data file path */
    char dataPath[MAX_PATH_LEN];
    if (job->isLoadProfile)
        snprintf(dataPath, sizeof(dataPath), "%s%d_payload.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);
    else
        snprintf(dataPath, sizeof(dataPath), "%s%d_readout.txt", METER_DATA_OUTPUT_DIR, (int)job->taskId);

    /* Ensure push connected */
    if (!appTcpConnManagerIsConnectedPush())
    {
        appTcpConnManagerRequestPushConnect();

        U32 t0 = osGetSystemTime();
        while ((osGetSystemTime() - t0) < PROTOCOL_CONNECT_TIMEOUT_MS)
        {
            if (appTcpConnManagerIsConnectedPush())
                break;
            zosDelayTask(100);
        }
        if (!appTcpConnManagerIsConnectedPush())
        {
            DEBUG_ERROR("->[E] Proto: push connect timeout, data delivery skipped");
            return;
        }
    }

    /* Send file data in packets */
    sendFilePacketised(PUSH_TCP_SOCK_NAME, funcName, meterId, dataPath);

    /* Wait for ACK (best-effort; files deleted regardless) */
    gs_pushRx.ready = FALSE;
    U32 t0 = osGetSystemTime();
    while ((osGetSystemTime() - t0) < PROTOCOL_ACK_TIMEOUT_MS)
    {
        if (gs_pushRx.ready)
        {
            gs_pushRx.ready = FALSE;
            break;
        }
        zosDelayTask(100);
    }
}

static void processPendingMeterJobs(void)
{
    for (int i = 0; i < MAX_PENDING_JOBS; i++)
    {
        if (gs_pendingJobs[i].taskId == 0)
            continue;
        if (!gs_pendingJobs[i].completed)
            continue;

        if (gs_pendingJobs[i].result == EN_ERR_CODE_SUCCESS)
        {
            deliverMeterData(i);
        }
        else
        {
            DEBUG_WARNING("->[W] Proto: meter task %d failed (err %d)",
                          gs_pendingJobs[i].taskId, gs_pendingJobs[i].result);
        }

        cleanupMeterFiles(gs_pendingJobs[i].taskId);
        appMeterOperationsTaskIDFree(gs_pendingJobs[i].taskId);
        gs_pendingJobs[i].taskId = 0;
        gs_pendingJobs[i].completed = FALSE;
    }
}

/* ------------------------------------------------------------------ */
/*                     Protocol task                                  */
/* ------------------------------------------------------------------ */

static void protocolTaskFunc(void *arg)
{
    (void)arg;
    DEBUG_INFO("->[I] %s task started", PROTO_TASK_NAME);
    appTskMngImOK(gs_taskId);

    /* --- Start TCP --- */
    if (0 != appTcpConnManagerStart(gs_serverIP, gs_serverPort, gs_pullPort, appProtocolZDPutIncomingMessage))
    {
        DEBUG_ERROR("->[E] Proto: TCP conn manager start failed");
        APP_LOG_REC(g_sysLoggerID, "Proto: TCP start fail");
        gs_running = FALSE;
        return;
    }
    appTcpConnManagerRequestConnect();
    zosDelayTask(500);

    /* --- Registration phase --- */
    gs_registered = registerStateLoad();

    if (!gs_registered)
    {
        DEBUG_INFO("->[I] Proto: device not registered, starting ident");

        while (gs_running && !gs_registered)
        {
            appTcpConnManagerRequestPushConnect();

            /* Wait for push connection */
            U32 t0 = osGetSystemTime();
            while ((osGetSystemTime() - t0) < PROTOCOL_CONNECT_TIMEOUT_MS && gs_running)
            {
                if (appTcpConnManagerIsConnectedPush())
                    break;
                zosDelayTask(100);
                appTskMngImOK(gs_taskId);
            }

            if (!appTcpConnManagerIsConnectedPush())
            {
                DEBUG_WARNING("->[W] Proto: push connect timeout, retrying in %d s", PROTOCOL_REGISTER_RETRY_S);
                for (int w = 0; w < PROTOCOL_REGISTER_RETRY_S * 10 && gs_running; w++)
                {
                    zosDelayTask(100);
                    appTskMngImOK(gs_taskId);
                }
                continue;
            }

            /* Send ident */
            int sz = buildIdentMsg(gs_txBuf, sizeof(gs_txBuf));
            appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
            DEBUG_DEBUG("->[D] Proto: ident sent (%d bytes)", sz);

            /* Wait for server response */
            gs_pushRx.ready = FALSE;
            t0 = osGetSystemTime();
            while ((osGetSystemTime() - t0) < (U32)PROTOCOL_REGISTER_RETRY_S * 1000U && gs_running)
            {
                if (gs_pushRx.ready)
                {
                    char funcStr[20] = {0};
                    ZDJson_GetStringValue(gs_pushRx.data, "function", funcStr, sizeof(funcStr));

                    if (0 == strcmp(funcStr, "ident"))
                    {
                        char resp[128];
                        if (ZDJson_GetObjectValue(gs_pushRx.data, "response", resp, sizeof(resp)))
                        {
                            bool regVal = false;
                            if (ZDJson_GetBoolValue(resp, "register", &regVal) && regVal)
                            {
                                gs_registered = TRUE;
                                registerStateSave(TRUE);
                                DEBUG_INFO("->[I] Proto: device registered successfully");
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
                DEBUG_DEBUG("->[D] Proto: registration attempt failed, retrying");
                for (int w = 0; w < PROTOCOL_REGISTER_RETRY_S * 10 && gs_running; w++)
                {
                    zosDelayTask(100);
                    appTskMngImOK(gs_taskId);
                }
            }
        }
    }
    else
    {
        DEBUG_INFO("->[I] Proto: device already registered");
    }

    /* --- Main loop (registered) --- */
    U32 aliveTimer = osGetSystemTime();

    while (gs_running)
    {
        appTskMngImOK(gs_taskId);

        /* Keep push socket alive */
        if (!appTcpConnManagerIsConnectedPush())
        {
            appTcpConnManagerRequestPushConnect();
        }

        /* ---- Alive ---- */
        U32 now = osGetSystemTime();
        if ((now - aliveTimer) >= (U32)PROTOCOL_ALIVE_INTERVAL_S * 1000U)
        {
            if (appTcpConnManagerIsConnectedPush())
            {
                int sz = buildAliveMsg(gs_txBuf, sizeof(gs_txBuf));
                appTcpConnManagerSend(PUSH_TCP_SOCK_NAME, gs_txBuf, (unsigned int)sz);
                DEBUG_DEBUG("->[D] Proto: alive sent");
            }
            aliveTimer = now;
        }

        /* ---- Pull requests ---- */
        if (gs_pullRx.ready)
        {
            processPullMessage(gs_pullRx.data);
            gs_pullRx.ready = FALSE;
        }

        /* ---- Push responses (alive ACK etc. – best effort) ---- */
        if (gs_pushRx.ready)
        {
            gs_pushRx.ready = FALSE;
        }

        /* ---- Completed meter tasks ---- */
        processPendingMeterJobs();

        zosDelayTask(100);
    }

    appTcpConnManagerStop();
    DEBUG_INFO("->[I] %s task stopped", PROTO_TASK_NAME);
}

/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appProtocolZDInit(const char *serialNumber)
{
    if (NULL == serialNumber)
        return FAILURE;

    strncpy(gs_serialNumber, serialNumber, sizeof(gs_serialNumber) - 1);

    /* load persisted connection config; fall back to defaults */
    if (FAILURE == connConfigLoad())
    {
        connConfigDefault();
        connConfigSave();
    }

    memset(&gs_pushRx, 0, sizeof(gs_pushRx));
    memset(&gs_pullRx, 0, sizeof(gs_pullRx));
    memset(gs_pendingJobs, 0, sizeof(gs_pendingJobs));

    gs_registered = registerStateLoad();
    gs_running = FALSE;
    gs_taskId = OS_INVALID_TASK_ID;

    return SUCCESS;
}

RETURN_STATUS appProtocolZDStart(void)
{
    if (gs_running)
        return SUCCESS;

    gs_running = TRUE;

    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = PROTO_TASK_STACK_SIZE;

    gs_taskId = appTskMngCreate(PROTO_TASK_NAME, protocolTaskFunc, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_taskId)
    {
        gs_running = FALSE;
        DEBUG_ERROR("->[E] Proto: task creation failed");
        APP_LOG_REC(g_sysLoggerID, "Proto task creation failed");
        return FAILURE;
    }

    DEBUG_INFO("->[I] Protocol ZD started");
    APP_LOG_REC(g_sysLoggerID, "Protocol ZD started");
    return SUCCESS;
}

RETURN_STATUS appProtocolZDStop(void)
{
    if (!gs_running)
        return SUCCESS;

    gs_running = FALSE;
    zosDelayTask(500);

    if (OS_INVALID_TASK_ID != gs_taskId)
    {
        appTskMngDelete(gs_taskId);
        gs_taskId = OS_INVALID_TASK_ID;
    }

    DEBUG_INFO("->[I] Protocol ZD stopped");
    APP_LOG_REC(g_sysLoggerID, "Protocol ZD stopped");
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*          Incoming TCP data callback (called from TCP thread)       */
/* ------------------------------------------------------------------ */

void appProtocolZDPutIncomingMessage(const char *channel,
                                         const char *data,
                                         unsigned int dataLength)
{
    if (channel == NULL || data == NULL || dataLength == 0)
        return;

    if (dataLength > PROTOCOL_PACKET_MAX_SIZE)
        dataLength = PROTOCOL_PACKET_MAX_SIZE;

    if (0 == strcmp(channel, PUSH_TCP_SOCK_NAME))
    {
        memcpy(gs_pushRx.data, data, dataLength);
        gs_pushRx.data[dataLength] = '\0';
        gs_pushRx.length = dataLength;
        gs_pushRx.ready  = TRUE;
    }
    else if (0 == strcmp(channel, PULL_TCP_SOCK_NAME))
    {
        memcpy(gs_pullRx.data, data, dataLength);
        gs_pullRx.data[dataLength] = '\0';
        gs_pullRx.length = dataLength;
        gs_pullRx.ready  = TRUE;
    }
}

/******************************** End Of File *********************************/
