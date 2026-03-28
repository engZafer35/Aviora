/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Mar 28, 2026
* #File Name    : AppMeterOperations.c
*******************************************************************************/
/******************************************************************************
 * Electric meter operations service: meter registry (serial-number keyed files),
 * IEC 62056-21 readout/profile jobs over MeterCommInterface_t, directive storage.
 ******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppMeterOperations.h"
#include "AppTaskManager.h"
#include "ZDJson.h"
#include "fs_port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef METER_OPS_TASK_STACK
#define METER_OPS_TASK_STACK  (3072U)
#endif

#define MAX_DIRECTIVES       (64)
#define MAX_METER_TASKS      (32)
#define METER_OPS_QUEUE_DEPTH (16)
#define DIRECTIVE_INDEX_FILE DIRECTIVE_MAIN_DIR "directive.idx"
#define MAX_DIRECTIVE_BODY   (8192)
#define MAX_PATH_LEN         (96)
#define IEC_LINE_TIMEOUT_MS  (5000U)
#define IEC_BYTE_TIMEOUT_MS  (500U)
#define METER_OPS_TASK_NAME  "MeterOpsSvc"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef enum
{
    TASK_PHASE_FREE = 0,
    TASK_PHASE_QUEUED,
    TASK_PHASE_RUNNING,
    TASK_PHASE_FINISHED,
} TaskPhase_t;

typedef struct
{
    S32 taskId;
    TaskPhase_t phase;
    ERR_CODE_T result;
} TaskSlot_t;

typedef enum
{
    METER_JOB_READOUT = 0,
    METER_JOB_PROFILE = 1,
} MeterJobKind_t;

typedef struct
{
    MeterJobKind_t kind;
    S32 taskId;
    char serial[20];
    char tStart[12];
    char tEnd[12];
    Callback_t callback;
} MeterJobMsg_t;

/********************************** VARIABLES *********************************/
static MeterCommInterface_t *s_meterIf = NULL;
static OsTaskId s_workerTid = OS_INVALID_TASK_ID;
static OsQueue s_jobQueue = OS_INVALID_QUEUE;
static bool s_running = false;

static U32 s_meterCount = 0;

/* Directive index table */
static char s_directiveIds[MAX_DIRECTIVES][MAX_KEY_LENGTH];
static U32 s_directiveCount = 0;

/* Task slots */
static TaskSlot_t s_taskSlots[MAX_METER_TASKS];
static S32 s_nextTaskId = 1;

/* Mutexes */
static OsMutex s_meterRegMux;
static OsMutex s_directiveMux;
static OsMutex s_taskMux;

/* Shared read buffers (mutex-protected) */
static char s_directiveReadBuf[MAX_DIRECTIVE_BODY];
static OsMutex s_getDirectiveMux;

/***************************** STATIC FUNCTIONS  ******************************/

static void lockMeterReg(void)   { zosAcquireMutex(&s_meterRegMux); }
static void unlockMeterReg(void) { zosReleaseMutex(&s_meterRegMux); }
static void lockDirective(void)  { zosAcquireMutex(&s_directiveMux); }
static void unlockDirective(void){ zosReleaseMutex(&s_directiveMux); }
static void lockTask(void)       { zosAcquireMutex(&s_taskMux); }
static void unlockTask(void)     { zosReleaseMutex(&s_taskMux); }

/* ---- Task slot management ---- */

static int taskSlotFindLocked(S32 taskId)
{
    for (U32 i = 0; i < MAX_METER_TASKS; i++)
    {
        if (s_taskSlots[i].taskId == taskId)
            return (int)i;
    }
    return -1;
}

static S32 taskSlotAlloc(void)
{
    lockTask();
    for (U32 i = 0; i < MAX_METER_TASKS; i++)
    {
        if (s_taskSlots[i].phase == TASK_PHASE_FREE)
        {
            S32 id = s_nextTaskId++;
            if (s_nextTaskId <= 0)
                s_nextTaskId = 1;
            s_taskSlots[i].taskId = id;
            s_taskSlots[i].phase = TASK_PHASE_QUEUED;
            s_taskSlots[i].result = EN_ERR_CODE_PENDING;
            unlockTask();
            return id;
        }
    }
    unlockTask();
    return (S32)EN_ERR_CODE_NO_RESOURCES;
}

static void taskSlotAbort(S32 taskId)
{
    lockTask();
    int ix = taskSlotFindLocked(taskId);
    if (ix >= 0)
    {
        s_taskSlots[ix].taskId = 0;
        s_taskSlots[ix].phase = TASK_PHASE_FREE;
        s_taskSlots[ix].result = EN_ERR_CODE_SUCCESS;
    }
    unlockTask();
}

static void taskSlotMarkRunning(S32 taskId)
{
    lockTask();
    int ix = taskSlotFindLocked(taskId);
    if (ix >= 0)
        s_taskSlots[ix].phase = TASK_PHASE_RUNNING;
    unlockTask();
}

static void taskSlotMarkFinished(S32 taskId, ERR_CODE_T res)
{
    lockTask();
    int ix = taskSlotFindLocked(taskId);
    if (ix >= 0)
    {
        s_taskSlots[ix].phase = TASK_PHASE_FINISHED;
        s_taskSlots[ix].result = res;
    }
    unlockTask();
}

/* ---- FS helpers ---- */

static error_t fsWriteWholeFile(const char_t *path, const void *data, size_t len)
{
    FsFile *f = fsOpenFile(path, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return ERROR_FILE_OPENING_FAILED;
    error_t e = fsWriteFile(f, (void *)data, len);
    fsCloseFile(f);
    return e;
}

static error_t fsReadWholeText(const char_t *path, char *buf, size_t cap, size_t *outLen)
{
    FsFile *f = fsOpenFile(path, FS_FILE_MODE_READ);
    if (f == NULL)
        return ERROR_FILE_NOT_FOUND;
    size_t total = 0;
    for (;;)
    {
        if (total + 1 >= cap)
            break;
        size_t chunk = 0;
        error_t e = fsReadFile(f, buf + total, cap - 1U - total, &chunk);
        if (e == ERROR_END_OF_FILE)
            break;
        if (e != NO_ERROR && chunk == 0)
        {
            fsCloseFile(f);
            return e;
        }
        total += chunk;
        if (chunk == 0)
            break;
    }
    buf[total] = '\0';
    if (outLen != NULL)
        *outLen = total;
    fsCloseFile(f);
    return NO_ERROR;
}

/* ---- Meter list (file-only, no full RAM mirror) ---- */

#define METER_LIST_ROW_SZ  (sizeof(MeterTable_t))

static void meterListRefreshCount(void)
{
    uint32_t sz = 0;
    if (fsGetFileSize((char_t *)METER_LIST_FILE, &sz) != NO_ERROR || sz == 0)
    {
        s_meterCount = 0;
        return;
    }
    s_meterCount = sz / (uint32_t)METER_LIST_ROW_SZ;
}

static BOOL meterListContainsSerial(const char serialKey[16])
{
    U8 buf[METER_LIST_ROW_SZ * 32];

    if (s_meterCount == 0)
        return FALSE;

    FsFile *f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FALSE;

    for (;;)
    {
        size_t got = 0;
        (void)fsReadFile(f, buf, sizeof(buf), &got);
        if (got == 0)
            break;

        size_t nRows = got / METER_LIST_ROW_SZ;
        for (size_t m = 0; m < nRows; m++)
        {
            if (memcmp(buf + m * METER_LIST_ROW_SZ, serialKey, METER_LIST_ROW_SZ) == 0)
            {
                fsCloseFile(f);
                return TRUE;
            }
        }
        if (got < sizeof(buf))
            break;
    }
    fsCloseFile(f);
    return FALSE;
}

static error_t meterListAppendRow(const MeterTable_t *row)
{
    FsFile *f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
    if (f == NULL)
        return ERROR_FILE_OPENING_FAILED;
    if (fsSeekFile(f, 0, FS_SEEK_END) != NO_ERROR)
    {
        fsCloseFile(f);
        return ERROR_FAILURE;
    }
    error_t w = fsWriteFile(f, (void *)row, sizeof(*row));
    fsCloseFile(f);
    return w;
}

static error_t meterListRemoveSerial(const char serialKey[16])
{
    if (s_meterCount == 0)
        return NO_ERROR;

    FsFile *src = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (src == NULL)
        return ERROR_FILE_OPENING_FAILED;

    const char_t *tmpPath = METER_MAIN_DIR "meterList.tmp";
    FsFile *dst = fsOpenFile(tmpPath, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (dst == NULL)
    {
        fsCloseFile(src);
        return ERROR_FILE_OPENING_FAILED;
    }

    MeterTable_t row;
    U32 written = 0;
    BOOL found = FALSE;

    for (;;)
    {
        size_t got = 0;
        (void)fsReadFile(src, &row, sizeof(row), &got);
        if (got < sizeof(row))
            break;
        if (memcmp(row.serialNumber, serialKey, METER_LIST_ROW_SZ) == 0)
        {
            found = TRUE;
            continue;
        }
        (void)fsWriteFile(dst, &row, sizeof(row));
        written++;
    }
    fsCloseFile(src);
    fsCloseFile(dst);

    if (!found)
    {
        (void)fsDeleteFile(tmpPath);
        return NO_ERROR;
    }

    (void)fsDeleteFile((char_t *)METER_LIST_FILE);
    if (written > 0)
        (void)fsRenameFile(tmpPath, (char_t *)METER_LIST_FILE);
    else
        (void)fsDeleteFile(tmpPath);

    s_meterCount = written;
    return NO_ERROR;
}

/* ---- Directive index helpers ---- */

static void directiveSafeBasename(const char *id, char *out, size_t outSz)
{
    size_t j = 0;
    for (size_t i = 0; id[i] != '\0' && j + 1 < outSz; i++)
    {
        unsigned char c = (unsigned char)id[i];
        if (isalnum(c) || c == '_' || c == '-')
            out[j++] = (char)c;
        else
            out[j++] = '_';
    }
    out[j] = '\0';
    if (strlen(out) > 40U)
        out[40] = '\0';
}

static void directiveFilePath(const char *id, char *path, size_t pathSz)
{
    char base[48];
    directiveSafeBasename(id, base, sizeof(base));
    snprintf(path, pathSz, "%s%s.json", DIRECTIVE_MAIN_DIR, base);
    path[pathSz - 1] = '\0';
}

static RETURN_STATUS directiveIndexSave(void)
{
    U32 magic = 0x44495258u;
    U8 hdr[sizeof(U32) * 2];
    memcpy(hdr, &magic, sizeof(magic));
    memcpy(hdr + sizeof(U32), &s_directiveCount, sizeof(U32));

    FsFile *f = fsOpenFile((char_t *)DIRECTIVE_INDEX_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return FAILURE;

    error_t e = fsWriteFile(f, hdr, sizeof(hdr));
    if (e == NO_ERROR && s_directiveCount > 0)
        e = fsWriteFile(f, s_directiveIds, (size_t)s_directiveCount * MAX_KEY_LENGTH);
    fsCloseFile(f);
    return (e == NO_ERROR) ? SUCCESS : FAILURE;
}

static RETURN_STATUS directiveIndexLoad(void)
{
    U8 hdr[sizeof(U32) * 2];
    s_directiveCount = 0;

    FsFile *f = fsOpenFile((char_t *)DIRECTIVE_INDEX_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return SUCCESS;

    size_t got = 0;
    error_t e = fsReadFile(f, hdr, sizeof(hdr), &got);
    if (e != NO_ERROR || got < sizeof(hdr))
    {
        fsCloseFile(f);
        return SUCCESS;
    }

    U32 magic, count;
    memcpy(&magic, hdr, sizeof(U32));
    memcpy(&count, hdr + sizeof(U32), sizeof(U32));

    if (magic != 0x44495258u || count > MAX_DIRECTIVES)
    {
        fsCloseFile(f);
        return SUCCESS;
    }

    got = 0;
    (void)fsReadFile(f, s_directiveIds, (size_t)count * MAX_KEY_LENGTH, &got);
    fsCloseFile(f);

    if (got >= (size_t)count * MAX_KEY_LENGTH)
        s_directiveCount = count;

    return SUCCESS;
}

static S32 directiveIndexFind(const char *id)
{
    for (U32 i = 0; i < s_directiveCount; i++)
    {
        if (strcmp(s_directiveIds[i], id) == 0)
            return (S32)i;
    }
    return -1;
}

/* ---- JSON helpers ---- */

static BOOL getJsonParamString(const char *json, const char *key, char *out, size_t outSz)
{
    char params[320];
    if (ZDJson_GetObjectValue(json, "parameters", params, sizeof(params)))
    {
        if (ZDJson_GetStringValue(params, key, out, outSz))
            return TRUE;
    }
    return ZDJson_GetStringValue(json, key, out, outSz);
}

static BOOL parseProfileWindow(const char *ts, char out11[11])
{
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
    if (sscanf(ts, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se) >= 5)
        ;
    else if (sscanf(ts, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se) >= 5)
        ;
    else
        return FALSE;
    if (mo < 1 || mo > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 || mi > 59)
        return FALSE;
    int yy = y % 100;
    if (y >= 0 && y < 100)
        yy = y;
    snprintf(out11, 11, "%02d%02d%02d%02d%02d", yy, mo, d, h, mi);
    return TRUE;
}

/* ---- IEC 62056 comm helpers ---- */

static int sendAll(const U8 *data, U32 len, U32 timeoutMs)
{
    if (s_meterIf == NULL || data == NULL || len == 0)
        return -1;
    S32 r = s_meterIf->meterCommSend(data, len, timeoutMs);
    return ((U32)r == len) ? 0 : -1;
}

static ERR_CODE_T recvLine(char *line, size_t cap, U32 overallTimeoutMs)
{
    if (s_meterIf == NULL || line == NULL || cap < 2)
        return EN_ERR_CODE_METER_COMM_LINE_PARAM_ERROR;

    size_t n = 0;
    systime_t t0 = osGetSystemTime();

    while (n + 1 < cap)
    {
        U8 b;
        U32 got = 1;
        S32 rr = s_meterIf->meterCommReceive(&b, &got, IEC_BYTE_TIMEOUT_MS);
        if (rr < 0)
            return EN_ERR_CODE_METER_COMM_LINE_ERROR;
        if (got == 1)
        {
            if (b == (U8)'\n')
            {
                line[n] = '\0';
                if (n > 0 && line[n - 1] == '\r')
                    line[n - 1] = '\0';
                return EN_ERR_CODE_SUCCESS;
            }
            line[n++] = (char)b;
        }
        if ((U32)(osGetSystemTime() - t0) > overallTimeoutMs)
            return EN_ERR_CODE_TIMEOUT;
    }
    return EN_ERR_CODE_METER_COMM_LINE_ERROR;
}

/* ---- IEC 62056 readout / profile ---- */

static ERR_CODE_T iecSaveMeterIdFile(S32 taskId, const char *line)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    return (fsWriteWholeFile((char_t *)path, line, strlen(line)) == NO_ERROR)
               ? EN_ERR_CODE_SUCCESS
               : EN_ERR_CODE_FAILURE;
}

/**
 * IEC 62056-21 readout flow (TCP meter sim):
 *  1. Send /?!\r\n  →  receive /identification\r\n  →  save as taskId_meterID.txt
 *  2. Send ACK050\r\n  →  receive OBIS lines until "!"  →  save as taskId_readout.txt
 */
static ERR_CODE_T iecDoReadout(S32 taskId)
{
    char pathOut[MAX_PATH_LEN];
    snprintf(pathOut, sizeof(pathOut), "%s%d_readout.txt", METER_DATA_OUTPUT_DIR, (int)taskId);

    if (sendAll((const U8 *)"/?!\r\n", 5U, IEC_LINE_TIMEOUT_MS) != 0)
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    char line[384];
    ERR_CODE_T e = recvLine(line, sizeof(line), IEC_LINE_TIMEOUT_MS);
    if (e != EN_ERR_CODE_SUCCESS)
        return e;
    if (line[0] != '/')
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    e = iecSaveMeterIdFile(taskId, line);
    if (e != EN_ERR_CODE_SUCCESS)
        return e;

    if (sendAll((const U8 *)"ACK050\r\n", 8U, IEC_LINE_TIMEOUT_MS) != 0)
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    FsFile *f = fsOpenFile((char_t *)pathOut, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return EN_ERR_CODE_FAILURE;

    for (;;)
    {
        e = recvLine(line, sizeof(line), IEC_LINE_TIMEOUT_MS * 3U);
        if (e != EN_ERR_CODE_SUCCESS)
        {
            fsCloseFile(f);
            return e;
        }
        if (line[0] == '\0')
            continue;
        if (line[0] == '!' && line[1] == '\0')
            break;
        fsWriteFile(f, line, strlen(line));
        fsWriteFile(f, (void *)"\n", 1);
    }
    fsCloseFile(f);
    return EN_ERR_CODE_SUCCESS;
}

/**
 * IEC 62056 load profile flow (TCP meter sim):
 *  1. Send /?!\r\n  →  identification  →  save as taskId_meterID.txt
 *  2. Send P.01(start)(end)\r\n  →  profile lines until "!"  →  save as taskId_payload.txt
 */
static ERR_CODE_T iecDoProfile(S32 taskId, const char *t0, const char *t1)
{
    char pathPay[MAX_PATH_LEN];
    snprintf(pathPay, sizeof(pathPay), "%s%d_payload.txt", METER_DATA_OUTPUT_DIR, (int)taskId);

    char cmd[80];
    snprintf(cmd, sizeof(cmd), "P.01(%s)(%s)\r\n", t0, t1);

    if (sendAll((const U8 *)"/?!\r\n", 5U, IEC_LINE_TIMEOUT_MS) != 0)
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    char line[384];
    ERR_CODE_T e = recvLine(line, sizeof(line), IEC_LINE_TIMEOUT_MS);
    if (e != EN_ERR_CODE_SUCCESS)
        return e;
    if (line[0] != '/')
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    e = iecSaveMeterIdFile(taskId, line);
    if (e != EN_ERR_CODE_SUCCESS)
        return e;

    if (sendAll((const U8 *)cmd, (U32)strlen(cmd), IEC_LINE_TIMEOUT_MS) != 0)
        return EN_ERR_CODE_METER_COMM_LINE_ERROR;

    FsFile *f = fsOpenFile((char_t *)pathPay, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return EN_ERR_CODE_FAILURE;

    for (;;)
    {
        e = recvLine(line, sizeof(line), IEC_LINE_TIMEOUT_MS * 6U);
        if (e != EN_ERR_CODE_SUCCESS)
        {
            fsCloseFile(f);
            return e;
        }
        if (line[0] == '\0')
            continue;
        if (strcmp(line, "!") == 0)
            break;
        fsWriteFile(f, line, strlen(line));
        fsWriteFile(f, (void *)"\n", 1);
    }
    fsCloseFile(f);
    return EN_ERR_CODE_SUCCESS;
}

/* ---- Worker task ---- */

static S32 enqueueMeterJob(const MeterJobMsg_t *job)
{
    if (s_jobQueue == OS_INVALID_QUEUE || !s_running)
        return (S32)EN_ERR_CODE_FAILURE;
    if (QUEUE_SUCCESS != zosMsgQueueSend(s_jobQueue, (const char *)job, sizeof(*job), TIME_OUT_10MS))
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    return job->taskId;
}

static void meterOpsWorkerTask(void *arg)
{
    (void)arg;
    MeterJobMsg_t job;

    DEBUG_INFO("->[I] %s worker started", METER_OPS_TASK_NAME);
    appTskMngImOK(s_workerTid);

    while (s_running)
    {
        if (QUEUE_SUCCESS == zosMsgQueueReceive(s_jobQueue, (char *)&job, sizeof(job), TIME_OUT_50MS))
        {
            if (s_meterIf == NULL)
            {
                if (job.callback != NULL)
                    job.callback(job.taskId, EN_ERR_CODE_FAILURE);
                taskSlotMarkFinished(job.taskId, EN_ERR_CODE_FAILURE);
                continue;
            }

            taskSlotMarkRunning(job.taskId);
            ERR_CODE_T res = EN_ERR_CODE_FAILURE;

            if (job.kind == METER_JOB_READOUT)
                res = iecDoReadout(job.taskId);
            else if (job.kind == METER_JOB_PROFILE)
                res = iecDoProfile(job.taskId, job.tStart, job.tEnd);

            taskSlotMarkFinished(job.taskId, res);
            if (job.callback != NULL)
                job.callback(job.taskId, res);
        }
        appTskMngImOK(s_workerTid);
    }

    DEBUG_WARNING("->[W] %s worker stopping", METER_OPS_TASK_NAME);
    zosMsgQueueClose(s_jobQueue);
    s_jobQueue = OS_INVALID_QUEUE;
    appTskMngDelete(&s_workerTid);
}

/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appMeterOperationsStart(MeterCommInterface_t *meterComm)
{
    if (meterComm == NULL)
        return FAILURE;
    if (s_running)
        return SUCCESS;

    s_meterIf = meterComm;

    if ((zosCreateMutex(&s_meterRegMux) == FALSE) || (zosCreateMutex(&s_directiveMux) == FALSE)
        || (zosCreateMutex(&s_taskMux) == FALSE) || (zosCreateMutex(&s_getDirectiveMux) == FALSE))
        return FAILURE;

    lockMeterReg();
    meterListRefreshCount();
    unlockMeterReg();

    if (meterComm->meterCommInit != NULL)
    {
        if (meterComm->meterCommInit() != SUCCESS)
            return FAILURE;
    }

    lockDirective();
    directiveIndexLoad();
    unlockDirective();

    for (U32 i = 0; i < MAX_METER_TASKS; i++)
    {
        s_taskSlots[i].taskId = 0;
        s_taskSlots[i].phase = TASK_PHASE_FREE;
        s_taskSlots[i].result = EN_ERR_CODE_SUCCESS;
    }

    s_running = true;

    s_jobQueue = zosMsgQueueCreate(QUEUE_NAME("MeterOpsQ"), METER_OPS_QUEUE_DEPTH, sizeof(MeterJobMsg_t));
    if (s_jobQueue == OS_INVALID_QUEUE)
    {
        s_running = false;
        return FAILURE;
    }

    ZOsTaskParameters taskParam;
    taskParam.priority = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = METER_OPS_TASK_STACK;

    s_workerTid = appTskMngCreate(METER_OPS_TASK_NAME, meterOpsWorkerTask, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == s_workerTid)
    {
        zosMsgQueueClose(s_jobQueue);
        s_jobQueue = OS_INVALID_QUEUE;
        s_running = false;
        return FAILURE;
    }

    DEBUG_INFO("->[I] Meter operations service started");
    return SUCCESS;
}

/* ---- Meter registration ---- */

RETURN_STATUS appMeterOperationsAddMeter(MeterData_t *meterData)
{
    char path[MAX_PATH_LEN];
    MeterTable_t row;

    if (meterData == NULL || meterData->serialNumber[0] == '\0')
        return FAILURE;

    memset(&row, 0, sizeof(row));
    strncpy(row.serialNumber, meterData->serialNumber, sizeof(row.serialNumber) - 1);

    lockMeterReg();

    if (meterListContainsSerial(row.serialNumber))
    {
        unlockMeterReg();
        return SUCCESS;
    }

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, meterData->serialNumber);
    error_t e = fsWriteWholeFile((char_t *)path, meterData, sizeof(MeterData_t));
    if (e != NO_ERROR)
    {
        unlockMeterReg();
        return FAILURE;
    }

    e = meterListAppendRow(&row);
    if (e != NO_ERROR)
    {
        (void)fsDeleteFile((char_t *)path);
        unlockMeterReg();
        return FAILURE;
    }
    s_meterCount++;

    unlockMeterReg();
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsGetMeterData(const char *serialNumber, MeterData_t *meterDataOut)
{
    char path[MAX_PATH_LEN];

    if (serialNumber == NULL || serialNumber[0] == '\0' || meterDataOut == NULL)
        return FAILURE;

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, serialNumber);

    FsFile *f = fsOpenFile((char_t *)path, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    size_t got = 0;
    if (fsReadFile(f, meterDataOut, sizeof(MeterData_t), &got) != NO_ERROR || got != sizeof(MeterData_t))
    {
        fsCloseFile(f);
        return FAILURE;
    }
    fsCloseFile(f);
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsDeleteMeter(const char *serialNumber)
{
    char path[MAX_PATH_LEN];

    if (serialNumber == NULL || serialNumber[0] == '\0')
        return FAILURE;

    lockMeterReg();

    if (!meterListContainsSerial(serialNumber))
    {
        unlockMeterReg();
        return FAILURE;
    }

    (void)meterListRemoveSerial(serialNumber);

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, serialNumber);
    (void)fsDeleteFile((char_t *)path);

    unlockMeterReg();
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsDeleteAllMeters(void)
{
    char path[MAX_PATH_LEN];
    U8 buf[METER_LIST_ROW_SZ * 32];

    lockMeterReg();

    if (s_meterCount > 0)
    {
        FsFile *f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
        if (f != NULL)
        {
            for (;;)
            {
                size_t got = 0;
                (void)fsReadFile(f, buf, sizeof(buf), &got);
                if (got == 0)
                    break;

                size_t nRows = got / METER_LIST_ROW_SZ;
                for (size_t m = 0; m < nRows; m++)
                {
                    MeterTable_t row;
                    memcpy(&row, buf + m * METER_LIST_ROW_SZ, sizeof(row));
                    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, row.serialNumber);
                    (void)fsDeleteFile((char_t *)path);
                }
                if (got < sizeof(buf))
                    break;
            }
            fsCloseFile(f);
        }
    }

    (void)fsDeleteFile((char_t *)METER_LIST_FILE);
    s_meterCount = 0;

    unlockMeterReg();
    return SUCCESS;
}

S32 appMeterOperationsGetMeterCount(void)
{
    lockMeterReg();
    S32 n = (S32)s_meterCount;
    unlockMeterReg();
    return n;
}

/* ---- Task API ---- */

S32 appMeterOperationsAddReadoutTask(const char *request, Callback_t callback)
{
    if (request == NULL || callback == NULL)
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!ZDJson_IsValid(request))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char serial[20];
    if (!getJsonParamString(request, "METERSERIALNUMBER", serial, sizeof(serial)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    MeterData_t tmp;
    if (appMeterOperationsGetMeterData(serial, &tmp) != SUCCESS)
        return (S32)EN_ERR_CODE_METER_NOT_FOUND;

    S32 tid = taskSlotAlloc();
    if (tid < 0)
        return tid;

    MeterJobMsg_t job;
    memset(&job, 0, sizeof(job));
    job.kind = METER_JOB_READOUT;
    job.taskId = tid;
    strncpy(job.serial, serial, sizeof(job.serial) - 1);
    job.callback = callback;

    if (enqueueMeterJob(&job) != tid)
    {
        taskSlotAbort(tid);
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    }
    return tid;
}

S32 appMeterOperationsAddProfileTask(const char *request, Callback_t callback)
{
    if (request == NULL || callback == NULL)
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!ZDJson_IsValid(request))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char serial[20];
    if (!getJsonParamString(request, "METERSERIALNUMBER", serial, sizeof(serial)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char d0[40], d1[40];
    if (!getJsonParamString(request, "startDate", d0, sizeof(d0)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!getJsonParamString(request, "endDate", d1, sizeof(d1)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char w0[11], w1[11];
    if (!parseProfileWindow(d0, w0) || !parseProfileWindow(d1, w1))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    MeterData_t tmp;
    if (appMeterOperationsGetMeterData(serial, &tmp) != SUCCESS)
        return (S32)EN_ERR_CODE_METER_NOT_FOUND;

    S32 tid = taskSlotAlloc();
    if (tid < 0)
        return tid;

    MeterJobMsg_t job;
    memset(&job, 0, sizeof(job));
    job.kind = METER_JOB_PROFILE;
    job.taskId = tid;
    strncpy(job.serial, serial, sizeof(job.serial) - 1);
    memcpy(job.tStart, w0, sizeof(job.tStart));
    memcpy(job.tEnd, w1, sizeof(job.tEnd));
    job.callback = callback;

    if (enqueueMeterJob(&job) != tid)
    {
        taskSlotAbort(tid);
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    }
    return tid;
}

S32 appMeterOperationsAddObisTask(const char *request, Callback_t callback)
{
    (void)request;
    (void)callback;
    return (S32)EN_ERR_CODE_NOT_IMPLEMENTED;
}

S32 appMeterOperationsAddWriteTask(const char *request, Callback_t callback)
{
    (void)request;
    (void)callback;
    return (S32)EN_ERR_CODE_NOT_IMPLEMENTED;
}

ERR_CODE_T appMeterOperationsIsTaskDone(S32 taskID)
{
    if (taskID <= 0)
        return EN_ERR_CODE_FAILURE;
    lockTask();
    int ix = taskSlotFindLocked(taskID);
    if (ix < 0)
    {
        unlockTask();
        return EN_ERR_CODE_FAILURE;
    }
    TaskPhase_t ph = s_taskSlots[ix].phase;
    ERR_CODE_T r = s_taskSlots[ix].result;
    unlockTask();
    return (ph == TASK_PHASE_FINISHED) ? r : EN_ERR_CODE_PENDING;
}

void appMeterOperationsTaskIDFree(S32 taskID)
{
    lockTask();
    int ix = taskSlotFindLocked(taskID);
    if (ix < 0 || s_taskSlots[ix].phase != TASK_PHASE_FINISHED)
    {
        unlockTask();
        return;
    }
    s_taskSlots[ix].taskId = 0;
    s_taskSlots[ix].phase = TASK_PHASE_FREE;
    s_taskSlots[ix].result = EN_ERR_CODE_SUCCESS;
    unlockTask();
}

/* ---- Directive management ---- */

S32 appMeterOperationsAddDirective(const char *directive)
{
    if (directive == NULL || !ZDJson_IsValid(directive))
        return (S32)EN_ERR_CODE_DIRECTIVE_PARAM_ERROR;

    char id[MAX_KEY_LENGTH];
    if (!ZDJson_GetStringValue(directive, "id", id, sizeof(id)))
        return (S32)EN_ERR_CODE_DIRECTIVE_PARAM_ERROR;

    lockDirective();

    char path[MAX_PATH_LEN];
    directiveFilePath(id, path, sizeof(path));

    size_t dlen = strlen(directive);
    if (dlen >= MAX_DIRECTIVE_BODY)
    {
        unlockDirective();
        return (S32)EN_ERR_CODE_DIRECTIVE_PARAM_ERROR;
    }

    if (fsWriteWholeFile((char_t *)path, directive, dlen) != NO_ERROR)
    {
        unlockDirective();
        return (S32)EN_ERR_CODE_FAILURE;
    }

    S32 idx = directiveIndexFind(id);
    if (idx < 0)
    {
        if (s_directiveCount >= MAX_DIRECTIVES)
        {
            unlockDirective();
            return (S32)EN_ERR_CODE_NO_RESOURCES;
        }
        strncpy(s_directiveIds[s_directiveCount], id, MAX_KEY_LENGTH - 1);
        s_directiveIds[s_directiveCount][MAX_KEY_LENGTH - 1] = '\0';
        idx = (S32)s_directiveCount;
        s_directiveCount++;
    }

    if (directiveIndexSave() != SUCCESS)
    {
        unlockDirective();
        return (S32)EN_ERR_CODE_FAILURE;
    }
    unlockDirective();
    return idx;
}

RETURN_STATUS appMeterOperationsDeleteDirective(const char *directiveID)
{
    lockDirective();

    if (directiveID == NULL || strcmp(directiveID, "*") == 0)
    {
        for (U32 i = 0; i < s_directiveCount; i++)
        {
            char path[MAX_PATH_LEN];
            directiveFilePath(s_directiveIds[i], path, sizeof(path));
            (void)fsDeleteFile((char_t *)path);
        }
        s_directiveCount = 0;
        (void)fsDeleteFile((char_t *)DIRECTIVE_INDEX_FILE);
        unlockDirective();
        return SUCCESS;
    }

    S32 ix = directiveIndexFind(directiveID);
    if (ix < 0)
    {
        unlockDirective();
        return FAILURE;
    }

    char path[MAX_PATH_LEN];
    directiveFilePath(directiveID, path, sizeof(path));
    (void)fsDeleteFile((char_t *)path);

    for (U32 j = (U32)ix + 1; j < s_directiveCount; j++)
        memcpy(s_directiveIds[j - 1], s_directiveIds[j], MAX_KEY_LENGTH);
    s_directiveCount--;

    directiveIndexSave();
    unlockDirective();
    return SUCCESS;
}

const char *appMeterOperationsGetDirective(const char *directiveID)
{
    if (directiveID == NULL)
        return NULL;

    lockDirective();
    if (directiveIndexFind(directiveID) < 0)
    {
        unlockDirective();
        return NULL;
    }
    char path[MAX_PATH_LEN];
    directiveFilePath(directiveID, path, sizeof(path));
    unlockDirective();

    zosAcquireMutex(&s_getDirectiveMux);
    size_t got = 0;
    error_t e = fsReadWholeText((char_t *)path, s_directiveReadBuf, sizeof(s_directiveReadBuf), &got);
    zosReleaseMutex(&s_getDirectiveMux);

    return (e == NO_ERROR) ? s_directiveReadBuf : NULL;
}

const char *appMeterOperationsGetDirectiveByIndex(U32 index)
{
    char path[MAX_PATH_LEN];

    lockDirective();
    if (index >= s_directiveCount)
    {
        unlockDirective();
        return NULL;
    }
    directiveFilePath(s_directiveIds[index], path, sizeof(path));
    unlockDirective();

    zosAcquireMutex(&s_getDirectiveMux);
    size_t got = 0;
    error_t er = fsReadWholeText((char_t *)path, s_directiveReadBuf, sizeof(s_directiveReadBuf), &got);
    zosReleaseMutex(&s_getDirectiveMux);

    return (er == NO_ERROR) ? s_directiveReadBuf : NULL;
}

S32 appMeterOperationsGetDirectiveCount(void)
{
    lockDirective();
    S32 n = (S32)s_directiveCount;
    unlockDirective();
    return n;
}

/******************************** End Of File *********************************/
