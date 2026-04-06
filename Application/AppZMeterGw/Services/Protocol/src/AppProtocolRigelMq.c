/******************************************************************************
* #Author       : Zafer Satilmis
* #Hhype-man    : KRONIK - CANIN CEHENNEME V2
* #Revision     : 1.0
* #Date         : Apr 1, 2026
* #File Name    : AppProtocolRigelMq.c
*******************************************************************************/

/******************************************************************************
* JSON-over-MQTT protocol handler with session management for IoT meter gateway.
* Session-based message processing with MQTT communication.
******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppProtocolRigelMq.h"
#include "AppMeterOperations.h"
#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppTimeService.h"
#include "AppSWUpdate.h"
#include "ZDJson.h"
#include "fs_port.h"
#include "AppMqttConnService.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/****************************** MACRO DEFINITIONS *****************************/
#define RG_SESSION_TASK_STACK_SIZE  (4096U)
#define RG_SESSION_TASK_NAME        "RGMqtt"

#define RG_SESSION_MAX_PENDING_JOBS  (4)
#define RG_SESSION_MAX_PATH_LEN      (96)

#define RG_SESSION_FTP_FIELD_MAX     (64)
#define RG_SESSION_FTP_DEFAULT_PORT  (21)
#define RG_SESSION_FW_LOCAL_PATH     "/tmp/firmware.bin"

/******************************* TYPE DEFINITIONS *****************************/

typedef struct
{
    S32 taskId;
    volatile bool completed;
    volatile ERR_CODE_T result;
    bool isLoadProfile;
} RG_PendingJob_t;

#pragma pack(push, 1)
typedef struct
{
    uint32_t magic;
    uint32_t version;
    char     tcpIp[20];
    int32_t  tcpPort;
    char     mqttBroker[20];
    int32_t  mqttPort;
    char     mqttUser[32];
    char     mqttPass[32];
    char     mqttReqTopic[RG_SESSION_TOPIC_MAX];
    char     mqttResTopic[RG_SESSION_TOPIC_MAX];
} RG_ServerPersist_t;
#pragma pack(pop)

#define RG_SRV_PERSIST_MAGIC  (0x52475356u)  /* 'RGSV' */
#define RG_SRV_PERSIST_VER    (1u)

/********************************** VARIABLES *********************************/
static union FlagList
{
    unsigned int sendIdent : 1; 
    
    unsigned int reserved : 31;
} gsFlagList;

static char gs_serialNumber[20];
static char gs_brokerIP[20];
static int  gs_brokerPort;
static char gs_userName[32];
static char gs_password[32];
static char gs_deviceIP[20];
static int  gs_pullPort;

/** Runtime MQTT topics (defaults from \c RG_SESSION_*_TOPIC; overridden by \c setting / \c request.mqtt) */
static char gs_mqttRequestTopic[RG_SESSION_TOPIC_MAX];
static char gs_mqttResponseTopic[RG_SESSION_TOPIC_MAX];

static BOOL gs_registered = false;
static volatile BOOL gs_running = false;
static OsTaskId gs_taskId = OS_INVALID_TASK_ID;

static RG_Session_t gs_sessionTable[RG_SESSION_MAX_SESSIONS];
static RG_PendingJob_t gs_pendingJobs[RG_SESSION_MAX_PENDING_JOBS];

static char gs_txBuf[RG_SESSION_PACKET_MAX_SIZE];

static OsQueue gs_msgQueue;
static OsQueue gs_eventQueue;

static uint32_t gs_aliveCnt = 0;
static U32 gs_localTransNum = 0;

static void rgSessionProcessSessionMessage(RG_Session_t *session);
static void rgSessionProcessIdentSession(RG_Session_t *session);
static void rgSessionProcessAliveSession(RG_Session_t *session);
static void rgSessionProcessLogSession(RG_Session_t *session);
static void rgSessionProcessReadoutSession(RG_Session_t *session);
static void rgSessionProcessLoadProfileSession(RG_Session_t *session);
static void rgSessionProcessDirectiveListSession(RG_Session_t *session);
static void rgSessionProcessSettingSession(RG_Session_t *session);
static void rgSessionProcessFwUpdateSession(RG_Session_t *session);
static void rgSessionProcessDirectiveAddSession(RG_Session_t *session);
static void rgSessionProcessDirectiveDeleteSession(RG_Session_t *session);
static void rgSessionMqttIncomingCb(const char *topic, const char *data, unsigned int dataLength);
static void rgSessionMqttLinkCb(int isUp);
static int rgSessionMqttRestart(void);
static void rgLoadDefaultSettings(void);

/***************************** STATIC FUNCTIONS  ******************************/
static U32 nextTransNumber(void)
{
    gs_localTransNum++;
    if (gs_localTransNum == 0) gs_localTransNum = 1;
    return gs_localTransNum;
}
/* ------------------------------------------------------------------ */
/*                     Register-state persistence                     */
/* ------------------------------------------------------------------ */

static bool rgSessionRegisterStateLoad(void)
{
    FsFile *f = fsOpenFile((char_t *)RG_SESSION_REGISTER_FILE, FS_FILE_MODE_READ);
    if (f == NULL) {
        return false;
    }

    U8 val = 0;
    size_t got = 0;
    (void)fsReadFile(f, &val, 1, &got);
    fsCloseFile(f);

    return (got == 1 && val == 1) ? true : false;
}

static void rgSessionRegisterStateSave(bool reg)
{
    FsFile *f = fsOpenFile((char_t *)RG_SESSION_REGISTER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return;

    U8 val = reg ? 1 : 0;
    (void)fsWriteFile(f, &val, 1);
    fsCloseFile(f);
}

/* ------------------------------------------------------------------ */
/*                   MQTT settings persistence (on-disk v1 layout)       */
/* ------------------------------------------------------------------ */

/**
 * Load all runtime settings from \c RG_RIGEL_DEFAULT_* macros in \c AppProtocolRigelMq.h.
 * Mirrors fields applied by \c rgSessionServerPersistLoad when a valid persist record exists.
 */
static void rgLoadDefaultSettings(void)
{
    strncpy(gs_brokerIP, RG_RIGEL_DEFAULT_MQTT_BROKER_IP, sizeof(gs_brokerIP) - 1);
    gs_brokerIP[sizeof(gs_brokerIP) - 1] = '\0';
    gs_brokerPort = RG_RIGEL_DEFAULT_MQTT_BROKER_PORT;

    strncpy(gs_userName, RG_RIGEL_DEFAULT_MQTT_USER_NAME, sizeof(gs_userName) - 1);
    gs_userName[sizeof(gs_userName) - 1] = '\0';

    strncpy(gs_password, RG_RIGEL_DEFAULT_MQTT_PASSWORD, sizeof(gs_password) - 1);
    gs_password[sizeof(gs_password) - 1] = '\0';

    strncpy(gs_mqttRequestTopic, RG_RIGEL_DEFAULT_MQTT_REQUEST_TOPIC, sizeof(gs_mqttRequestTopic) - 1);
    gs_mqttRequestTopic[sizeof(gs_mqttRequestTopic) - 1] = '\0';
    strncpy(gs_mqttResponseTopic, RG_RIGEL_DEFAULT_MQTT_RESPONSE_TOPIC, sizeof(gs_mqttResponseTopic) - 1);
    gs_mqttResponseTopic[sizeof(gs_mqttResponseTopic) - 1] = '\0';

    strncpy(gs_deviceIP, RG_RIGEL_DEFAULT_DEVICE_IP, sizeof(gs_deviceIP) - 1);
    gs_deviceIP[sizeof(gs_deviceIP) - 1] = '\0';
    gs_pullPort = RG_RIGEL_DEFAULT_PULL_PORT;
}

static void rgSessionServerPersistSave(void)
{
    RG_ServerPersist_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = RG_SRV_PERSIST_MAGIC;
    rec.version = RG_SRV_PERSIST_VER;
    strncpy(rec.tcpIp, gs_deviceIP, sizeof(rec.tcpIp) - 1);
    rec.tcpIp[sizeof(rec.tcpIp) - 1] = '\0';
    rec.tcpPort = (int32_t)gs_pullPort;
    strncpy(rec.mqttBroker, gs_brokerIP, sizeof(rec.mqttBroker) - 1);
    rec.mqttPort = (int32_t)gs_brokerPort;
    strncpy(rec.mqttUser, gs_userName, sizeof(rec.mqttUser) - 1);
    strncpy(rec.mqttPass, gs_password, sizeof(rec.mqttPass) - 1);
    strncpy(rec.mqttReqTopic, gs_mqttRequestTopic, sizeof(rec.mqttReqTopic) - 1);
    strncpy(rec.mqttResTopic, gs_mqttResponseTopic, sizeof(rec.mqttResTopic) - 1);

    FsFile *f = fsOpenFile((char_t *)RG_SESSION_SERVER_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return;
    (void)fsWriteFile(f, &rec, sizeof(rec));
    fsCloseFile(f);
}

/**
 * Load MQTT / device settings from disk (v1: \c tcpIp / \c tcpPort store device IP and pull port).
 * Call after \c rgLoadDefaultSettings so missing fields keep defaults.
 * @return SUCCESS if a valid record was read and applied; FAILURE if missing, short read, or bad magic/version.
 */
static RETURN_STATUS rgSessionServerPersistLoad(void)
{
    FsFile *f = fsOpenFile((char_t *)RG_SESSION_SERVER_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    uint8_t buf[sizeof(RG_ServerPersist_t) + 4];
    size_t got = 0;
    (void)fsReadFile(f, buf, sizeof(buf), &got);
    fsCloseFile(f);

    if (got != sizeof(RG_ServerPersist_t))
        return FAILURE;

    RG_ServerPersist_t rec;
    memcpy(&rec, buf, sizeof(rec));
    if (rec.magic != RG_SRV_PERSIST_MAGIC || rec.version != RG_SRV_PERSIST_VER)
        return FAILURE;

    if (rec.mqttBroker[0] != '\0')
    {
        strncpy(gs_brokerIP, rec.mqttBroker, sizeof(gs_brokerIP) - 1);
        gs_brokerIP[sizeof(gs_brokerIP) - 1] = '\0';
    }
    if (rec.mqttPort > 0)
        gs_brokerPort = (int)rec.mqttPort;

    strncpy(gs_userName, rec.mqttUser, sizeof(gs_userName) - 1);
    gs_userName[sizeof(gs_userName) - 1] = '\0';
    strncpy(gs_password, rec.mqttPass, sizeof(gs_password) - 1);
    gs_password[sizeof(gs_password) - 1] = '\0';

    if (rec.mqttReqTopic[0] != '\0')
    {
        strncpy(gs_mqttRequestTopic, rec.mqttReqTopic, sizeof(gs_mqttRequestTopic) - 1);
        gs_mqttRequestTopic[sizeof(gs_mqttRequestTopic) - 1] = '\0';
    }
    if (rec.mqttResTopic[0] != '\0')
    {
        strncpy(gs_mqttResponseTopic, rec.mqttResTopic, sizeof(gs_mqttResponseTopic) - 1);
        gs_mqttResponseTopic[sizeof(gs_mqttResponseTopic) - 1] = '\0';
    }

    if (rec.tcpIp[0] != '\0')
    {
        strncpy(gs_deviceIP, rec.tcpIp, sizeof(gs_deviceIP) - 1);
        gs_deviceIP[sizeof(gs_deviceIP) - 1] = '\0';
    }
    if (rec.tcpPort > 0)
        gs_pullPort = (int)rec.tcpPort;

    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*                      FTP URL parser (ZD-compatible)                 */
/* ------------------------------------------------------------------ */

static bool rgParseFtpUrl(const char *url,
                          char *host, size_t hostSz,
                          U16 *port,
                          char *user, size_t userSz,
                          char *pass, size_t passSz,
                          char *path, size_t pathSz)
{
    if (strncmp(url, "ftp://", 6) != 0)
        return false;

    const char *p = url + 6;

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
        *port = (U16)RG_SESSION_FTP_DEFAULT_PORT;
    }

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

    return true;
}

/* ------------------------------------------------------------------ */
/*                          Time helper                               */
/* ------------------------------------------------------------------ */

static void rgSessionGetDeviceDateStr(char *buf, size_t bufSz)
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

static void rgSessionJsonAddDeviceHeader(RGJson_Builder_t *jb)
{
    ZDJson_OpenObject(jb, "device");
    ZDJson_AddString(jb, "flag", RG_SESSION_FLAG);
    ZDJson_AddString(jb, "serialNumber", gs_serialNumber);
    ZDJson_CloseObject(jb);
}

static int rgSessionBuildAckNack(char *buf, size_t sz, bool isAck)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    rgSessionJsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", isAck ? "ack" : "nack");
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

/**
 * Device-originated ident (→ \a avi/response): \c transNumber correlates the server reply on \a avi/request.
 * \c response.registered is local persistence hint; server reply uses the same key under \c response (see \a rgJsonResponseGetRegister).
 */
static int rgSessionBuildIdentMsg(char *buf, size_t sz, uint16_t transNum)
{
    char dateStr[24];
    rgSessionGetDeviceDateStr(dateStr, sizeof(dateStr));

    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    rgSessionJsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "ident");
    ZDJson_AddNumber(&jb, "transNumber", (int)transNum);
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddBool(&jb, "registered", (0 != gs_registered) ? true : false);
    ZDJson_AddString(&jb, "brand", RG_SESSION_BRAND);
    ZDJson_AddString(&jb, "model", RG_SESSION_MODEL);
    ZDJson_AddString(&jb, "deviceDate", dateStr);
    ZDJson_AddString(&jb, "pullIP", gs_deviceIP);
    ZDJson_AddNumber(&jb, "pullPort", gs_pullPort);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int rgSessionBuildAliveMsg(char *buf, size_t sz)
{
    char dateStr[24];
    rgSessionGetDeviceDateStr(dateStr, sizeof(dateStr));

    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    rgSessionJsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "alive");
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddString(&jb, "deviceDate", dateStr);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int rgSessionBuildLogPacket(char *buf, size_t sz,
                                   const char *logData, int packetNum, bool stream)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    rgSessionJsonAddDeviceHeader(&jb);
    ZDJson_AddString(&jb, "function", "log");
    ZDJson_AddNumber(&jb, "packetNum", packetNum);
    ZDJson_AddBool(&jb, "packetStream", stream ? true : false);
    ZDJson_OpenObject(&jb, "response");
    ZDJson_AddString(&jb, "log", logData);
    ZDJson_CloseObject(&jb);
    ZDJson_CloseObject(&jb);
    return (int)jb.pos;
}

static int rgSessionBuildDataPacket(char *buf, size_t sz,
                                    const char *funcName,
                                    const char *meterId,
                                    const char *data,
                                    int packetNum, bool stream)
{
    ZDJson_Builder_t jb;
    ZDJson_Init(&jb, buf, sz);
    ZDJson_OpenObject(&jb, NULL);
    rgSessionJsonAddDeviceHeader(&jb);
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

static int rgSessionBuildDirectivePacket(char *buf, size_t sz,
                                         const char *directive,
                                         int packetNum, bool stream, int transNum)
{
    return snprintf(buf, sz,
        "{\"device\":{\"flag\":\"%s\",\"serialNumber\":\"%s\"},"
        "\"function\":\"directiveList\","
        "\"transNumber\":%d,"
        "\"packetNum\":%d,"
        "\"packetStream\":%s,"
        "\"response\":{\"directive\":{%s}}}",
        RG_SESSION_FLAG, gs_serialNumber,
        transNum,
        packetNum,
        stream ? "true" : "false",
        directive);
}

/* ------------------------------------------------------------------ */
/*               Packetised send helpers                              */
/* ------------------------------------------------------------------ */

static void rgSessionSendLogPackets(void)
{
    char curBuf[RG_SESSION_DATA_CHUNK_SIZE + 1];
    char nxtBuf[RG_SESSION_DATA_CHUNK_SIZE + 1];

    S32 curLen = appLogRecRead(g_sysLoggerID, curBuf, RG_SESSION_DATA_CHUNK_SIZE);

    if (curLen <= 0)
    {
        int sz = rgSessionBuildLogPacket(gs_txBuf, sizeof(gs_txBuf), "NO-LOG", 1, false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        return;
    }

    int packetNum = 0;

    while (curLen > 0)
    {
        curBuf[curLen] = '\0';
        packetNum++;

        S32 nxtLen = appLogRecRead(g_sysLoggerID, nxtBuf, RG_SESSION_DATA_CHUNK_SIZE);
        bool hasMore = (nxtLen > 0);

        int sz = rgSessionBuildLogPacket(gs_txBuf, sizeof(gs_txBuf), curBuf, packetNum, hasMore);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);

        if (!hasMore)
            break;

        memcpy(curBuf, nxtBuf, (size_t)nxtLen);
        curLen = nxtLen;
    }
}

static void rgSessionSendFilePacketised(const char *funcName,
                                        const char *meterId,
                                        const char *filePath)
{
    FsFile *f = fsOpenFile((char_t *)filePath, FS_FILE_MODE_READ);
    if (f == NULL)
        return;

    char bufA[RG_SESSION_DATA_CHUNK_SIZE + 1];
    char bufB[RG_SESSION_DATA_CHUNK_SIZE + 1];
    char *cur = bufA;
    char *nxt = bufB;

    size_t curLen = 0;
    (void)fsReadFile(f, cur, RG_SESSION_DATA_CHUNK_SIZE, &curLen);

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
        (void)fsReadFile(f, nxt, RG_SESSION_DATA_CHUNK_SIZE, &nxtLen);
        bool hasMore = (nxtLen > 0);

        int sz = rgSessionBuildDataPacket(gs_txBuf, sizeof(gs_txBuf),
                                          funcName, meterId, cur,
                                          packetNum, hasMore);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);

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
/*                     Session management                             */
/* ------------------------------------------------------------------ */
static void rgCleanupMeterFiles(S32 taskId)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_readout.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_payload.txt",  METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
}

static void rgMeterOperationsOnComplete(RG_Session_t *session, RG_SessionState_t state, void *arg)
{    
    //if (SESSION_DONE == state)
    {
        if ((session->pendingJobIdx >= 0) && (session->pendingJobIdx < RG_SESSION_MAX_PENDING_JOBS))
        {
            RG_PendingJob_t *job = &gs_pendingJobs[session->pendingJobIdx];

            rgCleanupMeterFiles(job->taskId);
            appMeterOperationsTaskIDFree(job->taskId);

            memset(job, 0, sizeof(*job));
            session->pendingJobIdx = -1;
        }
    }
}

static RG_Session_t *rgSessionCreate(uint8_t function, uint16_t transNum, RG_SessionCompleteCb_t onComplete)
{
    for (unsigned int i = 0; i < RG_SESSION_MAX_SESSIONS; i++)
    {
        if (!gs_sessionTable[i].inUse)
        {
            RG_Session_t *s = &gs_sessionTable[i];
            memset(s, 0, sizeof(*s));
            s->inUse = true;
            s->function = function;
            s->transNumber = transNum;
            s->step = 0;
            s->timeCnt = 0;
            s->pendingJobIdx = -1;
            s->rxReady = false;
            s->rxLen = 0;
            s->onCompleteFunc = onComplete;
            return s;
        }
    }
    return NULL;
}

static RG_Session_t *rgSessionFindByTransNumber(uint16_t transNum)
{
    for (unsigned int i = 0; i < RG_SESSION_MAX_SESSIONS; i++)
    {
        if (gs_sessionTable[i].inUse && gs_sessionTable[i].transNumber == transNum)
            return &gs_sessionTable[i];
    }
    return NULL;
}

static void rgSessionDelete(RG_Session_t *s)
{
    if (s == NULL)
        return;

    if (s->onCompleteFunc)
        s->onCompleteFunc(s, RG_SESSION_STATE_DONE, NULL);

    memset(s, 0, sizeof(RG_Session_t));
}

static void rgCompleteSession(RG_Session_t *session, RG_SessionState_t state, void *arg)
{
    if (session->onCompleteFunc)
        session->onCompleteFunc(session, state, arg);
}

static void rgSessionDispatchIncomingMessage(const RG_SessionMsgQueue_t *msg)
{
    if (msg == NULL || msg->payload[0] == '\0')
        return;

    // Validate JSON
    if (!ZDJson_IsValid(msg->payload))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        return;
    }

    // Parse JSON to extract function and transNumber
    uint8_t func = 0;

    // Get function string
    RG_SessionCompleteCb_t onComplete = NULL;
    char funcStr[32] = {0};
    if (ZDJson_GetStringValue(msg->payload, "function", funcStr, sizeof(funcStr)))
    {
        if (strcmp(funcStr, "ident") == 0) 
        {
            func = RG_FUNC_IDENT;
        } 
        else if (strcmp(funcStr, "alive") == 0)
        {
            func = RG_FUNC_ALIVE;
        }
        else if (strcmp(funcStr, "log") == 0)
        {
            func = RG_FUNC_LOG;
        } 
        else if (strcmp(funcStr, "readout") == 0) 
        {
            func = RG_FUNC_READOUT;
            onComplete = rgMeterOperationsOnComplete;
        } 
        else if (strcmp(funcStr, "loadprofile") == 0)
        {
            func = RG_FUNC_LOADPROFILE;
            onComplete = rgMeterOperationsOnComplete;
        }
        else if (strcmp(funcStr, "directiveList") == 0) 
        {
            func = RG_FUNC_DIRECTIVE_LIST;
        }
        else if (strcmp(funcStr, "setting") == 0)
        {
            func = RG_FUNC_SETTING;
        }
        else if (strcmp(funcStr, "fwUpdate") == 0)
        {
            func = RG_FUNC_FWUPDATE;
        }
        else if (strcmp(funcStr, "directiveAdd") == 0)
        {
            func = RG_FUNC_DIRECTIVE_ADD;
        }
        else if (strcmp(funcStr, "directiveDelete") == 0)
        {
            func = RG_FUNC_DIRECTIVE_DELETE;
        }
        else 
        {
            // Unknown function
            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            return;
        }
    }
    else
    {
        // No function field
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        return;
    }

    // Get transNumber
    int tempTransNum = 0;
    if (FALSE == ZDJson_GetNumberValue(msg->payload, "transNumber", &tempTransNum))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        return;
    }

    // If transNum belongs to an active session, route it
    RG_Session_t *session = (tempTransNum != 0) ? rgSessionFindByTransNumber(tempTransNum) : NULL;
    if (session) 
    {
        // Set incoming data to session
        strncpy(session->rxBuf, msg->payload, sizeof(session->rxBuf) - 1);
        session->rxLen = (uint16_t)strlen(session->rxBuf);
        session->rxReady = true;
        rgSessionProcessSessionMessage(session);
        return;
    }

    // Start new session
    session = rgSessionCreate(func, tempTransNum, onComplete);
    if (session) 
    {
        // Set incoming data
        strncpy(session->rxBuf, msg->payload, sizeof(session->rxBuf) - 1);
        session->rxLen = (uint16_t)strlen(session->rxBuf);
        session->rxReady = true;
        rgSessionProcessSessionMessage(session);
    } else 
    {
        // Too many sessions
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    }
}

static void rgSessionMqttIncomingCb(const char *topic, const char *data, unsigned int dataLength)
{
    if (strcmp(topic, gs_mqttRequestTopic) != 0 || dataLength == 0 || dataLength >= RG_SESSION_PACKET_MAX_SIZE)
        return;

    RG_SessionMsgQueue_t msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.ch, gs_mqttRequestTopic, sizeof(msg.ch) - 1);
    memcpy(msg.payload, data, dataLength);
    msg.payload[dataLength] = '\0';

    zosMsgQueueSend(gs_msgQueue, (const char *)&msg, sizeof(msg), 0);
}

static void rgSessionMqttLinkCb(int isUp)
{
    if (isUp)
    {
        /* MQTT connection established */
    }
    else
    {
        /* MQTT disconnected */
    }
}

static int rgSessionMqttRestart(void)
{
    appMqttConnServiceRequestDisconnect();
    zosDelayTask(3000);

    (void)appMqttConnServiceStop();
    zosDelayTask(3000);
    return appMqttConnServiceStart(gs_brokerIP, gs_brokerPort,
                                   gs_userName[0] ? gs_userName : NULL,
                                   gs_password[0] ? gs_password : NULL,
                                   gs_mqttRequestTopic,
                                   rgSessionMqttIncomingCb,
                                   rgSessionMqttLinkCb);
}

static void rgSessionProcessSessionMessage(RG_Session_t *session)
{
    switch (session->function)
    {
        case RG_FUNC_IDENT:
            rgSessionProcessIdentSession(session);
            break;
        case RG_FUNC_ALIVE:
            rgSessionProcessAliveSession(session);
            break;
        case RG_FUNC_LOG:
            rgSessionProcessLogSession(session);
            break;
        case RG_FUNC_READOUT:
            rgSessionProcessReadoutSession(session);
            break;
        case RG_FUNC_LOADPROFILE:
            rgSessionProcessLoadProfileSession(session);
            break;
        case RG_FUNC_DIRECTIVE_LIST:
            rgSessionProcessDirectiveListSession(session);
            break;
        case RG_FUNC_SETTING:
            rgSessionProcessSettingSession(session);
            break;
        case RG_FUNC_FWUPDATE:
            rgSessionProcessFwUpdateSession(session);
            break;
        case RG_FUNC_DIRECTIVE_ADD:
            rgSessionProcessDirectiveAddSession(session);
            break;
        case RG_FUNC_DIRECTIVE_DELETE:
            rgSessionProcessDirectiveDeleteSession(session);
            break;
        default:
            // Unknown function
            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            rgSessionDelete(session);
            break;
    }
}

static void rgSessionTask(void *arg)
{
    (void)arg;
    unsigned int i;

    // Initialize MQTT service
    int ret = appMqttConnServiceStart(gs_brokerIP, gs_brokerPort,
                                      gs_userName[0] ? gs_userName : NULL,
                                      gs_password[0] ? gs_password : NULL,
                                      gs_mqttRequestTopic,
                                      rgSessionMqttIncomingCb,
                                      rgSessionMqttLinkCb);
    if (ret != 0)
    {
        gs_running = false;
        return;
    }

    // Request connection
    appMqttConnServiceRequestConnect();

    zosDelayTask(5000); // Wait for connection to establish

    // Main loop
    while (gs_running)
    {
        // Check for send alive flag
        if ((TRUE == gs_registered) && (gs_aliveCnt++ >= RG_SESSION_ALIVE_INTERVAL_S))
        {
            if (NULL != rgSessionCreate(RG_FUNC_ALIVE, nextTransNumber(), NULL))
            {
                gs_aliveCnt = 0; // Try again in next interval
            }
        }
        
        // Check for send register flag
        if (TRUE == gsFlagList.sendIdent)
        {
            if (NULL != rgSessionCreate(RG_FUNC_IDENT, nextTransNumber(), NULL))
            {
                gsFlagList.sendIdent = FALSE;
            }
        }

        // Check session timeouts
        for (i = 0; i < RG_SESSION_MAX_SESSIONS; i++)
        {
            if (gs_sessionTable[i].inUse)
            {
                gs_sessionTable[i].timeCnt += 100; // Assume 100ms tick
                if (gs_sessionTable[i].timeCnt >= RG_SESSION_TIMEOUT_MS)
                {
                    rgCompleteSession(&gs_sessionTable[i], RG_SESSION_STATE_TIMEOUT, NULL);                    
                    rgSessionDelete(&gs_sessionTable[i]);
                }
            }
        }

        // Check message queue
        RG_SessionMsgQueue_t msg;
        if (zosMsgQueueReceive(gs_msgQueue, (char *)&msg, sizeof(msg), 0) == 0)
        {
            rgSessionDispatchIncomingMessage(&msg);
        }

        /* Process all active sessions */
        for (i = 0; i < RG_SESSION_MAX_SESSIONS; i++)
        {
            if (gs_sessionTable[i].inUse)
                rgSessionProcessSessionMessage(&gs_sessionTable[i]);
        }

        // Check event queue
        RG_SessionEventType_t event;
        if (zosMsgQueueReceive(gs_eventQueue, (char *)&event, sizeof(event), 0) == 0)
        {
            rgSessionProcessEvent(event);
        }

        zosDelayTask(100); // 100ms delay
    }

    // Cleanup
    appMqttConnServiceStop();
}

static int rgSessionPendingJobAllocSlot(S32 taskId, bool isLoadProfile)
{
    for (int i = 0; i < RG_SESSION_MAX_PENDING_JOBS; i++)
    {
        if (gs_pendingJobs[i].taskId == 0)
        {
            gs_pendingJobs[i].taskId = taskId;
            gs_pendingJobs[i].completed = false;
            gs_pendingJobs[i].result = EN_ERR_CODE_SUCCESS;
            gs_pendingJobs[i].isLoadProfile = isLoadProfile;
            return i;
        }
    }
    return -1;
}

static void rgSessionMeterTaskCallback(S32 taskID, ERR_CODE_T status)
{
    int slot = -1;
    for (int i = 0; i < RG_SESSION_MAX_PENDING_JOBS; i++)
    {
        if (gs_pendingJobs[i].taskId == taskID)
        {
            slot = i;
            break;
        }
    }
    if (slot >= 0)
    {
        gs_pendingJobs[slot].completed = true;
        gs_pendingJobs[slot].result = status;
    }
}

static void rgSessionCleanupMeterFiles(S32 taskId)
{
    char path[RG_SESSION_MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_readout.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
    snprintf(path, sizeof(path), "%s%d_payload.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    (void)fsDeleteFile((char_t *)path);
}

static void rgSessionProcessEvent(RG_SessionEventType_t event)
{
    switch (event)
    {
        case RG_SESSION_EVENT_ALIVE:
        {
            int sz = rgSessionBuildAliveMsg(gs_txBuf, sizeof(gs_txBuf));
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            break;
        }
        case RG_SESSION_EVENT_FW_UPGRADE_SUCCESS:
        {
            // Handle FW upgrade success event if needed
            break;
        }
        default:
            break;
    }
}

/**
 * Orion \c processIdentSession analogue: step 0 = publish device ident on \a avi/response;
 * step 1 = wait for server JSON on \a avi/request with matching \a transNumber.
 */
static void rgSessionProcessIdentSession(RG_Session_t *session)
{
    switch (session->step)
    {
        case 0:
        {
            int sz = rgSessionBuildIdentMsg(gs_txBuf, sizeof(gs_txBuf), session->transNumber);
            if (0 != appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz))
            {
                EBUG_WARNING("->[W] RigelMq: pushident send failed");
                APP_LOG_REC(g_sysLoggerID, "RigelMq ident publish failed");

                gsFlagList.sendIdent = TRUE; // Retry later

                rgSessionDelete(session);
                return;
            }
            session->step = 1;
            session->timeCnt = 0U;
            break;
        }
        case 1:
        {
            if (!session->rxReady) return;

            if (ZDJson_GetBoolValue(inner, "registered", &gs_registered))
            {
                rgSessionRegisterStateSave(gs_registered);            
            }

            gsFlagList.sendIdent = !gs_registered;

            session->rxReady = false;
            session->rxLen = 0U;
            rgSessionDelete(session);
            break;
        }

        default:
            rgSessionDelete(session);
            break;
    }
}

static void rgSessionProcessAliveSession(RG_Session_t *session)
{
    int sz = rgSessionBuildAliveMsg(gs_txBuf, sizeof(gs_txBuf));
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    rgSessionDelete(session);
}

static void rgSessionProcessLogSession(RG_Session_t *session)
{
    rgSessionSendLogPackets();
    rgSessionDelete(session);
}

static void rgSessionProcessReadoutSession(RG_Session_t *session)
{
    switch (session->step)
    {
        case 0:
        {
            // Parse directive and meterId
            char directive[64] = {0}, meterId[64] = {0};
            ZDJson_GetStringValue(session->rxBuf, "directive", directive, sizeof(directive));
            ZDJson_GetStringValue(session->rxBuf, "meterId", meterId, sizeof(meterId));

            if (directive[0] == '\0' || meterId[0] == '\0')
            {
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            char reqBuf[320];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\"}}",
                     directive, meterId);

            S32 tid = appMeterOperationsAddReadoutTask(reqBuf, rgSessionMeterTaskCallback);
            if (tid <= 0)
            {
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            int slot = rgSessionPendingJobAllocSlot(tid, false);
            if (slot < 0)
            {
                appMeterOperationsTaskIDFree(tid);
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            session->pendingJobIdx = slot;

            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), true);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);

            session->step = 1;
            session->timeCnt = 0;

            break;
        }

        case 1:
        {
            if (session->pendingJobIdx >= 0 && gs_pendingJobs[session->pendingJobIdx].completed)
            {
                int idx = session->pendingJobIdx;
                S32 tid = gs_pendingJobs[idx].taskId;
                ERR_CODE_T res = gs_pendingJobs[idx].result;

                if (res == EN_ERR_CODE_SUCCESS)
                {
                    // Send file data
                    char meterId[64] = "default";
                    ZDJson_GetStringValue(session->rxBuf, "meterId", meterId, sizeof(meterId));
                    rgSessionSendFilePacketised("readout", meterId, "/path/to/readout/data");
                }
                else
                {
                    // Send NACK
                    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                }

                // Cleanup meter files and free task ID
                rgSessionCleanupMeterFiles(tid);
                appMeterOperationsTaskIDFree(tid);

                rgSessionDelete(session);
            }
            break;
        }

        default:
            rgSessionDelete(session);
            break;
    }
}

static void rgSessionProcessLoadProfileSession(RG_Session_t *session)
{
    switch (session->step)
    {
        case 0:
        {
            // Parse directive, meterId, startDate, endDate
            char directive[64] = {0}, meterId[64] = {0}, startDate[24] = {0}, endDate[24] = {0};
            ZDJson_GetStringValue(session->rxBuf, "directive", directive, sizeof(directive));
            ZDJson_GetStringValue(session->rxBuf, "meterId", meterId, sizeof(meterId));
            ZDJson_GetStringValue(session->rxBuf, "startDate", startDate, sizeof(startDate));
            ZDJson_GetStringValue(session->rxBuf, "endDate", endDate, sizeof(endDate));

            if (directive[0] == '\0' || meterId[0] == '\0')
            {
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            char reqBuf[384];
            snprintf(reqBuf, sizeof(reqBuf),
                     "{\"directive\":\"%s\",\"parameters\":{\"METERSERIALNUMBER\":\"%s\",\"STARTDATE\":\"%s\",\"ENDDATE\":\"%s\"}}",
                     directive, meterId, startDate, endDate);

            S32 tid = appMeterOperationsAddProfileTask(reqBuf, rgSessionMeterTaskCallback);
            if (tid <= 0)
            {
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            int slot = rgSessionPendingJobAllocSlot(tid, true);
            if (slot < 0)
            {
                appMeterOperationsTaskIDFree(tid);
                int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                rgSessionDelete(session);
                return;
            }

            session->pendingJobIdx = slot;

            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), true);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);

            session->step = 1;
            session->timeCnt = 0;

            break;
        }

        case 1:
        {
            if (session->pendingJobIdx >= 0 && gs_pendingJobs[session->pendingJobIdx].completed)
            {
                int idx = session->pendingJobIdx;
                S32 tid = gs_pendingJobs[idx].taskId;
                ERR_CODE_T res = gs_pendingJobs[idx].result;

                if (res == EN_ERR_CODE_SUCCESS)
                {
                    // Send file data
                    char meterId[64] = "load-profile-no-id";
                    ZDJson_GetStringValue(session->rxBuf, "meterId", meterId, sizeof(meterId));
                    rgSessionSendFilePacketised("loadprofile", meterId, "/path/to/loadprofile/data");
                }
                else
                {
                    // Send NACK
                    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
                    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
                }

                // Cleanup meter files and free task ID
                rgSessionCleanupMeterFiles(tid);
                appMeterOperationsTaskIDFree(tid);

                rgSessionDelete(session);
            }
            break;
        }

        default:
            rgSessionDelete(session);
            break;
    }
}

static void rgSessionProcessSettingSession(RG_Session_t *session)
{ 
    char reqObj[1024];
    if (FALSE == ZDJson_GetObjectValue(session->rxBuf, "request", reqObj, sizeof(reqObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    char mqttObj[512];
    char tempStr[96];
    int tempVal;
    BOOL mqttCorrect = FALSE;

    if (ZDJson_GetObjectValue(reqObj, "mqtt", mqttObj, sizeof(mqttObj)))
    {
        tempStr[0] = '\0';
        if (ZDJson_GetStringValue(mqttObj, "broker", tempStr, sizeof(tempStr)) && tempStr[0] != '\0')
        {
            strncpy(gs_brokerIP, tempStr, sizeof(gs_brokerIP) - 1);
            gs_brokerIP[sizeof(gs_brokerIP) - 1] = '\0';
            mqttCorrect = TRUE;
            DEBUG_INFO("->[I] RigelMq: NEW MQTT broker IP: %s", gs_brokerIP);
        }

        tempVal = 0;
        if (mqttCorrect && (mqttCorrect = ZDJson_GetNumberValue(mqttObj, "port", &tempVal)))
        {            
            gs_brokerPort = tempVal;
            DEBUG_INFO("->[I] RigelMq: NEW MQTT broker port: %d", gs_brokerPort);
        }

        if (mqttCorrect && (mqttCorrect = ZDJson_GetStringValue(mqttObj, "userName", tempStr, sizeof(tempStr))))
        {            
            strncpy(gs_userName, tempStr, sizeof(gs_userName) - 1);
            gs_userName[sizeof(gs_userName) - 1] = '\0';
            DEBUG_INFO("->[I] RigelMq: NEW MQTT username: %s", gs_userName);
        }

        if (mqttCorrect && (mqttCorrect = ZDJson_GetStringValue(mqttObj, "password", tempStr, sizeof(tempStr))))
        {
            strncpy(gs_password, tempStr, sizeof(gs_password) - 1);
            gs_password[sizeof(gs_password) - 1] = '\0';
            DEBUG_INFO("->[I] RigelMq: NEW MQTT password: %s", gs_password);
        }

        tempStr[0] = '\0';
        if (mqttCorrect && (mqttCorrect = ZDJson_GetStringValue(mqttObj, "requestTopic", tempStr, sizeof(tempStr))))
        {
            strncpy(gs_mqttRequestTopic, tempStr, sizeof(gs_mqttRequestTopic) - 1);
            gs_mqttRequestTopic[sizeof(gs_mqttRequestTopic) - 1] = '\0';
            DEBUG_INFO("->[I] RigelMq: NEW MQTT request topic: %s", gs_mqttRequestTopic);
        }
        tempStr[0] = '\0';
        if (mqttCorrect && (mqttCorrect = ZDJson_GetStringValue(mqttObj, "responseTopic", tempStr, sizeof(tempStr))))
        {
            strncpy(gs_mqttResponseTopic, tempStr, sizeof(gs_mqttResponseTopic) - 1);
            gs_mqttResponseTopic[sizeof(gs_mqttResponseTopic) - 1] = '\0';
            DEBUG_INFO("->[I] RigelMq: NEW MQTT response topic: %s", gs_mqttResponseTopic);          
        }

        if (TRUE == mqttCorrect)
        {
            rgSessionServerPersistSave();
            if (0 != rgSessionMqttRestart())
            {
                // mqtt connection could not be restarted with new settings
                DEBUG_ERROR("->[E] RigelMq: MQTT connection restart failed with new settings");
                APP_LOG_REC(g_sysLoggerID, "MQTT connection restart failed with new settings");

                //TODO: restart the service or the device
            }
            else
            {
                appMqttConnServiceRequestConnect();
                DEBUG_INFO("->[I] RigelMq: MQTT settings updated and connection restarted");
                APP_LOG_REC(g_sysLoggerID, "MQTT settings updated and connection restarted");
            }
        }
        else
        {
            if (FAILURE == rgSessionServerPersistLoad())
            {
                DEBUG_WARNING("->[W] RigelMq: Revert — persisted MQTT settings could not be reloaded");
                //todo: LOAD DEFAULT SETTINGS
            }
            DEBUG_ERROR("->[E] RigelMq: Invalid MQTT settings in request");
            APP_LOG_REC(g_sysLoggerID, "RigelMq: Invalid MQTT settings in request");
            
            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            rgSessionDelete(session);
            return;
        }
    }

    int idx = 0;
    bool success = true;
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
                success = false;
        }
        else if (0 == strcmp(operation, "remove"))
        {
            char sn[20] = {0};
            ZDJson_GetStringValue(meterObj, "serialNumber", sn, sizeof(sn));
            if (sn[0] != '\0')
            {
                if (SUCCESS != appMeterOperationsDeleteMeter(sn))
                    success = false;
            }
        }

        idx++;
    }

    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), success);
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    rgSessionDelete(session);
}

static void rgSessionProcessFwUpdateSession(RG_Session_t *session)
{
    char reqObj[256];
    if (FALSE == ZDJson_GetObjectValue(session->rxBuf, "request", reqObj, sizeof(reqObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    char address[192] = {0};
    ZDJson_GetStringValue(reqObj, "address", address, sizeof(address));

    if (address[0] == '\0')
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), true);
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);

    char host[RG_SESSION_FTP_FIELD_MAX];
    char user[RG_SESSION_FTP_FIELD_MAX];
    char pass[RG_SESSION_FTP_FIELD_MAX];
    char path[RG_SESSION_FTP_FIELD_MAX];
    U16 port = (U16)RG_SESSION_FTP_DEFAULT_PORT;

    if (rgParseFtpUrl(address, host, sizeof(host), &port,
                      user, sizeof(user), pass, sizeof(pass),
                      path, sizeof(path)))
    {
        (void)AppSwUpdateInit(host, port, user, pass, path, RG_SESSION_FW_LOCAL_PATH);
       
        DEBUG_INFO("->[I] RigelMq: FW update started from %s", host);
        APP_LOG_REC(g_sysLoggerID, "FW update started from %s", host);
    }
    else
    {
        DEBUG_ERROR("->[E] RigelMq: FTP URL parse failed");
        APP_LOG_REC(g_sysLoggerID, "FTP URL parse failed");
    }

    rgSessionDelete(session);
}

static void rgSessionProcessDirectiveAddSession(RG_Session_t *session)
{
    char reqObj[RG_SESSION_PACKET_MAX_SIZE];
    if (FALSE == ZDJson_GetObjectValue(session->rxBuf, "request", reqObj, sizeof(reqObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    char dirObj[RG_SESSION_PACKET_MAX_SIZE];
    if (FALSE == ZDJson_GetObjectValue(reqObj, "directive", dirObj, sizeof(dirObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    S32 result = appMeterOperationsAddDirective(dirObj);
    bool ok = (result >= 0);

    DEBUG_INFO("->[I] RigelMq: DirectiveID %s ADDED result: %d", dirObj, ok);
    APP_LOG_REC(g_sysLoggerID, "RigelMq: DirectiveID %s ADDED result: %d", dirObj, ok);

    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    rgSessionDelete(session);
}

static void rgSessionProcessDirectiveDeleteSession(RG_Session_t *session)
{
    char reqObj[RG_SESSION_PACKET_MAX_SIZE];
    char filterObj[256];
    char filterId[MAX_KEY_LENGTH] = {0};

    if (FALSE == ZDJson_GetObjectValue(session->rxBuf, "request", reqObj, sizeof(reqObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    if (FALSE == ZDJson_GetObjectValue(reqObj, "filter", filterObj, sizeof(filterObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    ZDJson_GetStringValue(filterObj, "id", filterId, sizeof(filterId));

    if (filterId[0] == '\0')
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    /* filterId "*" → delete all directives (see appMeterOperationsDeleteDirective). */
    BOOL ok = (SUCCESS == appMeterOperationsDeleteDirective(filterId));

    DEBUG_INFO("->[I] RigelMq: DirectiveID %s DELETED result: %d", filterId, ok);
    APP_LOG_REC(g_sysLoggerID, "RigelMq: DirectiveID %s DELETED result: %d", filterId, ok);

    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), ok);
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    rgSessionDelete(session);
}

static void rgSessionProcessDirectiveListSession(RG_Session_t *session)
{
    char reqObj[RG_SESSION_PACKET_MAX_SIZE];
    char filterObj[256];
    char filterId[MAX_KEY_LENGTH] = "";
    char directiveBuff[RG_SESSION_PACKET_MAX_SIZE] = "";

    if (FALSE == ZDJson_GetObjectValue(session->rxBuf, "request", reqObj, sizeof(reqObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    if (FALSE == ZDJson_GetObjectValue(reqObj, "filter", filterObj, sizeof(filterObj)))
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    ZDJson_GetStringValue(filterObj, "id", filterId, sizeof(filterId));

    if ('\0' == filterId[0])
    {
        int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    /* id "*" → publish one response packet per directive (same transNumber). */
    if ('*' == filterId[0])
    {
        int count = appMeterOperationsGetDirectiveCount();
        if (count <= 0)
        {
            int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            rgSessionDelete(session);
            return;
        }

        for (int i = 0; i < count; i++)
        {
            appMeterOperationsGetDirectiveByIndex(i, directiveBuff, sizeof(directiveBuff));
            int sz = rgSessionBuildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), directiveBuff, 1, false, session->transNumber);
            appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
            zosDelayTask(500); /* avoid broker flood */
        }
        rgSessionDelete(session);
        return;
    }

    /* Single directive by id */
    if (SUCCESS == appMeterOperationsGetDirective(filterId, directiveBuff, sizeof(directiveBuff)))
    {
        int sz = rgSessionBuildDirectivePacket(gs_txBuf, sizeof(gs_txBuf), directiveBuff, 1, false, session->transNumber);
        appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
        rgSessionDelete(session);
        return;
    }

    int sz = rgSessionBuildAckNack(gs_txBuf, sizeof(gs_txBuf), false);
    appMqttConnServicePublish(gs_mqttResponseTopic, gs_txBuf, (unsigned int)sz);
    rgSessionDelete(session);
}
/************************* GLOBAL FUNCTION DEFINITIONS **************************/

RETURN_STATUS appProtocolRigelMqInit(const char *serialNumber)
{
    if (serialNumber == NULL)
        return FAILURE;

    strncpy(gs_serialNumber, serialNumber, sizeof(gs_serialNumber) - 1);
    gs_serialNumber[sizeof(gs_serialNumber) - 1] = '\0';

    if (FAILURE == rgSessionServerPersistLoad())
    {
        DEBUG_WARNING("->[W] RigelMq: Persisted MQTT settings could not be loaded");
        APP_LOG_REC(g_sysLoggerID, "RigelMq: Persisted MQTT settings could not be loaded");

        rgLoadDefaultSettings();
        rgSessionServerPersistSave();

        DEBUG_INFO("->[I] RigelMq: Default settings saved to " RG_SESSION_SERVER_FILE);
        APP_LOG_REC(g_sysLoggerID, "RigelMq: Default settings saved to " RG_SESSION_SERVER_FILE);
    }

    gs_registered = rgSessionRegisterStateLoad();

    memset(gs_sessionTable, 0, sizeof(gs_sessionTable));
    memset(gs_pendingJobs, 0, sizeof(gs_pendingJobs));

    gs_msgQueue = zosMsgQueueCreate("RGMsg", 10, sizeof(RG_SessionMsgQueue_t));
    if (gs_msgQueue == NULL) {
        return FAILURE;
    }

    gs_eventQueue = zosMsgQueueCreate("RGEvent", 5, sizeof(RG_SessionEventType_t));
    if (gs_eventQueue == NULL) {
        zosMsgQueueClose(gs_msgQueue);
        return FAILURE;
    }

    DEBUG_INFO("->[I] RigelMq: Initialized");
    return SUCCESS;
}

RETURN_STATUS appProtocolRigelMqStart(void)
{
    if (gs_running)
        return FAILURE;

    gs_running = true;

    gs_taskId = zosTaskCreate(RG_SESSION_TASK_NAME,
                              rgSessionTask,
                              NULL,
                              RG_SESSION_TASK_STACK_SIZE,
                              TASK_PRIORITY_NORMAL);

    if (gs_taskId == OS_INVALID_TASK_ID)
    {
        DEBUG_ERROR("->[E] RigelMq: Task creation failed");
        APP_LOG_REC(g_sysLoggerID, "RigelMq: Task creation failed");
        gs_running = false;
        return FAILURE;
    }

    DEBUG_INFO("->[I] RigelMq: Started");
    APP_LOG_REC(g_sysLoggerID, "RigelMq: Started");

    return SUCCESS;
}

RETURN_STATUS appProtocolRigelMqStop(void)
{
    if (!gs_running)
        return FAILURE;

    gs_running = false;

    if (gs_taskId != OS_INVALID_TASK_ID)
    {
        zosTaskDelete(gs_taskId);
        gs_taskId = OS_INVALID_TASK_ID;
    }

    appMqttConnServiceStop();

    if (gs_msgQueue != NULL) {
        zosMsgQueueClose(gs_msgQueue);
        gs_msgQueue = NULL;
    }

    if (gs_eventQueue != NULL) {
        zosMsgQueueClose(gs_eventQueue);
        gs_eventQueue = NULL;
    }

    DEBUG_WARNING("->[W] RigelMq: Stopped");
    APP_LOG_REC(g_sysLoggerID, "RigelMq: Stopped");

    return SUCCESS;
}

BOOL appProtocolRigelMqSendEvent(RG_SessionEventType_t type)
{
    if (gs_eventQueue == NULL || !gs_running)
        return FALSE;
    if (QUEUE_SUCCESS != zosMsgQueueSend(gs_eventQueue, (const char *)&type, sizeof(type), TIME_OUT_10MS))
        return FALSE;

    DEBUG_INFO("->[I] RigelMq: New event type %d", type);
    APP_LOG_REC(g_sysLoggerID, "RigelMq: New event %d", type);
    return TRUE;
}

/************************* GLOBAL FUNCTION DEFINITIONS **************************/