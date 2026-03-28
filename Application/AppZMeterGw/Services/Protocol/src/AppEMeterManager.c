/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 2.0
* #Date         : Mar 27, 2026
* #File Name    : AppEMeterManager.c
*******************************************************************************/
/******************************************************************************
 * Electric meter manager: meter registry (flash-friendly), IEC 62056-21 style
 * jobs over MeterCommInterface_t (e.g. TCP meter sim), directive storage, worker task.
 ******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppEMeterManager.h"
#include "AppTaskManager.h"
#include "ZDJson.h"
#include "fs_port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef EMETER_MANAGER_TASK_STACK
#define EMETER_MANAGER_TASK_STACK  (3072U)
#endif

#define MAX_DIRECTIVES       (64)
#define MAX_METER_TASKS      (32)
#define EMETER_QUEUE_DEPTH   (16)
#define DIRECTIVE_INDEX_FILE DIRECTIVE_MAIN_DIR "directive.idx"
#define MAX_PATH_LEN         (96)
#define IEC_LINE_TIMEOUT_MS  (5000U)
#define IEC_BYTE_TIMEOUT_MS  (500U)
#define METER_TASK_PREFIX    "EMeterSvc"

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
static bool s_managerRunning = false;

static char s_directiveIds[MAX_DIRECTIVES][MAX_KEY_LENGTH];
static U32 s_directiveCount = 0;

static TaskSlot_t s_taskSlots[MAX_METER_TASKS];
static S32 s_nextTaskId = 1;

static OsMutex s_meterRegMux;
static OsMutex s_directiveMux;
static OsMutex s_taskMux;

static char *s_directiveReadBuf = NULL;
static size_t s_directiveReadBufCap = 0;
static OsMutex s_getDirectiveMux;

/** RAM'de kayıtlı sayaç adedi; meterList dosya boyutu ile senkron (start'ta yüklenir). */
static U32 s_meterCount = 0;

/***************************** STATIC FUNCTIONS  ******************************/

static void lockMeterReg(void)  { zosAcquireMutex(&s_meterRegMux); }
static void unlockMeterReg(void) { zosReleaseMutex(&s_meterRegMux); }
static void lockDirective(void)  { zosAcquireMutex(&s_directiveMux); }
static void unlockDirective(void) { zosReleaseMutex(&s_directiveMux); }
static void lockTask(void)       { zosAcquireMutex(&s_taskMux); }
static void unlockTask(void)     { zosReleaseMutex(&s_taskMux); }

/** @pre s_taskMux held */
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

static const char *directiveReadFileToScratch(const char_t *path)
{
    uint32_t fsz = 0;
    if (fsGetFileSize((char_t *)path, &fsz) != NO_ERROR)
        return NULL;
    size_t need = (size_t)fsz + 1U;
    if (need == 0U)
        need = 1U;
    if (need > s_directiveReadBufCap)
    {
        char *nb = (char *)realloc(s_directiveReadBuf, need);
        if (nb == NULL)
            return NULL;
        s_directiveReadBuf = nb;
        s_directiveReadBufCap = need;
    }
    size_t got = 0;
    if (fsReadWholeText((char_t *)path, s_directiveReadBuf, s_directiveReadBufCap, &got) != NO_ERROR)
        return NULL;
    return s_directiveReadBuf;
}

#define METER_LIST_ROW_SZ  (sizeof(MeterTable_t))

static void meterListRefreshCountFromFile(void)
{
    uint32_t sz = 0;

    if (fsGetFileSize((char_t *)METER_LIST_FILE, &sz) != NO_ERROR)
    {
        s_meterCount = 0;
        return;
    }
    if (sz == 0U || (sz % (uint32_t)METER_LIST_ROW_SZ) != 0U)
    {
        s_meterCount = 0;
        return;
    }
    s_meterCount = sz / (uint32_t)METER_LIST_ROW_SZ;
}

static BOOL meterListContainsSerial(const char serialKey[16])
{
    unsigned char buf[1024];
    FsFile *f;

    if (s_meterCount == 0U)
        return FALSE;

    f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FALSE;

    for (;;)
    {
        size_t got = 0;

        (void)fsReadFile(f, buf, sizeof(buf), &got);

        if (got == 0U)
        {
            fsCloseFile(f);
            return FALSE;
        }

        size_t nMeters = got / METER_LIST_ROW_SZ;

        for (size_t m = 0; m < nMeters; m++)
        {
            if (memcmp(buf + m * METER_LIST_ROW_SZ, serialKey, 16) == 0)
            {
                fsCloseFile(f);
                return TRUE;
            }
        }

        if (got < sizeof(buf))
        {
            fsCloseFile(f);
            return FALSE;
        }
    }
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
    U32 n = s_meterCount;

    if (n == 0U)
        return NO_ERROR;

    size_t blob = (size_t)n * sizeof(MeterTable_t);
    MeterTable_t *buf = (MeterTable_t *)malloc(blob);

    if (buf == NULL)
        return ERROR_OUT_OF_MEMORY;

    FsFile *f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
    {
        free(buf);
        return ERROR_FILE_OPENING_FAILED;
    }
    size_t got = 0;
    error_t e = fsReadFile(f, buf, blob, &got);
    fsCloseFile(f);
    if (e != NO_ERROR || got != blob)
    {
        free(buf);
        return (e != NO_ERROR) ? e : ERROR_INVALID_PARAMETER;
    }

    size_t w = 0;
    for (U32 i = 0; i < n; i++)
    {
        if (memcmp(buf[i].serialNumber, serialKey, 16) != 0)
            buf[w++] = buf[i];
    }
    if (w == n)
    {
        free(buf);
        return NO_ERROR;
    }

    if (w == 0)
        e = fsDeleteFile((char_t *)METER_LIST_FILE);
    else
        e = fsWriteWholeFile((char_t *)METER_LIST_FILE, buf, w * sizeof(MeterTable_t));

    free(buf);
    if (e == NO_ERROR)
        s_meterCount = (U32)w;
    return e;
}

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
    size_t blob = sizeof(U32) * 2 + (size_t)s_directiveCount * MAX_KEY_LENGTH;
    U8 *buf = (U8 *)malloc(blob);
    if (buf == NULL)
        return FAILURE;
    memcpy(buf, &magic, sizeof(magic));
    memcpy(buf + sizeof(U32), &s_directiveCount, sizeof(U32));
    memcpy(buf + sizeof(U32) * 2, s_directiveIds, (size_t)s_directiveCount * MAX_KEY_LENGTH);
    error_t e = fsWriteWholeFile(DIRECTIVE_INDEX_FILE, buf, blob);
    free(buf);
    return (e == NO_ERROR) ? SUCCESS : FAILURE;
}

static RETURN_STATUS directiveIndexLoad(void)
{
    size_t cap = sizeof(U32) * 2 + (size_t)MAX_DIRECTIVES * MAX_KEY_LENGTH;
    U8 *buf = (U8 *)malloc(cap);
    if (buf == NULL)
        return FAILURE;
    size_t len = 0;
    error_t e = fsReadWholeText(DIRECTIVE_INDEX_FILE, (char *)buf, cap, &len);
    if (e != NO_ERROR || len < sizeof(U32) * 2)
    {
        free(buf);
        s_directiveCount = 0;
        return SUCCESS;
    }
    U32 magic = ((U32 *)buf)[0];
    U32 count = ((U32 *)buf)[1];
    if (magic != 0x44495258u || count > MAX_DIRECTIVES
        || len < sizeof(U32) * 2 + count * MAX_KEY_LENGTH)
    {
        free(buf);
        s_directiveCount = 0;
        return SUCCESS;
    }
    s_directiveCount = count;
    memcpy(s_directiveIds, buf + sizeof(U32) * 2, (size_t)count * MAX_KEY_LENGTH);
    free(buf);
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

static int sendAll(const U8 *data, U32 len, U32 timeoutMs)
{
    if (s_meterIf == NULL || data == NULL || len == 0)
        return -1;
    S32 r = s_meterIf->meterCommSend(data, len, timeoutMs);
    if (r < 0)
        return -1;
    if ((U32)r == len)
        return 0;
    if (r == 0 && len > 0)
        return -1;
    return -1;
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

static ERR_CODE_T iecSaveMeterIdFile(S32 taskId, const char *line)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    return (fsWriteWholeFile((char_t *)path, line, strlen(line)) == NO_ERROR)
               ? EN_ERR_CODE_SUCCESS
               : EN_ERR_CODE_FAILURE;
}

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

static void meterWorkerTask(void *arg)
{
    (void)arg;
    MeterJobMsg_t job;

    DEBUG_INFO("->[I] %s worker started", METER_TASK_PREFIX);
    appTskMngImOK(s_workerTid);

    while (s_managerRunning)
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

    DEBUG_WARNING("->[W] %s worker stopping", METER_TASK_PREFIX);
    zosMsgQueueClose(s_jobQueue);
    s_jobQueue = OS_INVALID_QUEUE;
    appTskMngDelete(&s_workerTid);
}

/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appMeterManagerStart(MeterCommInterface_t *meterComm)
{
    if (meterComm == NULL)
        return FAILURE;

    if (s_managerRunning)
        return SUCCESS;

    s_meterIf = meterComm;

    if ((zosCreateMutex(&s_meterRegMux) == FALSE) || (zosCreateMutex(&s_directiveMux) == FALSE)
        || (zosCreateMutex(&s_taskMux) == FALSE) || (zosCreateMutex(&s_getDirectiveMux) == FALSE))
        return FAILURE;

    lockMeterReg();
    meterListRefreshCountFromFile();
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

    s_managerRunning = true;

    s_jobQueue = zosMsgQueueCreate(QUEUE_NAME("MeterMgrQ"), EMETER_QUEUE_DEPTH, sizeof(MeterJobMsg_t));
    if (s_jobQueue == OS_INVALID_QUEUE)
    {
        s_managerRunning = false;
        return FAILURE;
    }

    ZOsTaskParameters taskParam;
    taskParam.priority = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = EMETER_MANAGER_TASK_STACK;

    s_workerTid = appTskMngCreate(METER_TASK_PREFIX, meterWorkerTask, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == s_workerTid)
    {
        zosMsgQueueClose(s_jobQueue);
        s_jobQueue = OS_INVALID_QUEUE;
        s_managerRunning = false;
        return FAILURE;
    }

    DEBUG_INFO("->[I] Meter manager started");
    return SUCCESS;
}

RETURN_STATUS appMeterAddMeter(MeterData_t *meterData)
{
    char path[MAX_PATH_LEN];
    MeterTable_t row;

    if (meterData == NULL || meterData->serialNumber[0] == '\0')
        return FAILURE;

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, meterData->serialNumber);
    memcpy(row.serialNumber, meterData->serialNumber, sizeof(row.serialNumber));

    lockMeterReg();

    if (meterListContainsSerial(row.serialNumber))
    {
        unlockMeterReg();
        return SUCCESS;
    }

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

RETURN_STATUS appMeterGetMeterData(const char *serialNumber, MeterData_t *meterDataOut)
{
    char path[MAX_PATH_LEN];
    FsFile *f;
    size_t got = 0;

    if (serialNumber == NULL || serialNumber[0] == '\0' || meterDataOut == NULL)
        return FAILURE;

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, serialNumber);

    f = fsOpenFile((char_t *)path, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    if (fsReadFile(f, meterDataOut, sizeof(MeterData_t), &got) != NO_ERROR || got != sizeof(MeterData_t))
    {
        fsCloseFile(f);
        return FAILURE;
    }
    fsCloseFile(f);
    return SUCCESS;
}

RETURN_STATUS appMeterDeleteMeter(const char *serialNumber)
{
    char path[MAX_PATH_LEN];
    char key[16];
    size_t k;

    if (serialNumber == NULL || serialNumber[0] == '\0')
        return FAILURE;

    for (k = 0; k < sizeof(key) && serialNumber[k] != '\0'; k++)
        key[k] = serialNumber[k];
    while (k < sizeof(key))
        key[k++] = '\0';

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, serialNumber);

    lockMeterReg();
    error_t lr = meterListRemoveSerial(key);
    if (lr == ERROR_OUT_OF_MEMORY)
    {
        unlockMeterReg();
        return FAILURE;
    }
    error_t del = fsDeleteFile((char_t *)path);
    unlockMeterReg();
    return (del == NO_ERROR) ? SUCCESS : FAILURE;
}

RETURN_STATUS appMeterDeleteAllMeters(void)
{
    char path[MAX_PATH_LEN];
    unsigned char buf[1024];
    U32 n;

    lockMeterReg();
    n = s_meterCount;
    if (n > 0U)
    {
        FsFile *f = fsOpenFile((char_t *)METER_LIST_FILE, FS_FILE_MODE_READ);
        if (f != NULL)
        {
            U32 processed = 0;
            const size_t totalBytes = (size_t)n * METER_LIST_ROW_SZ;

            while (processed < n)
            {
                size_t stillBytes = totalBytes - (size_t)processed * METER_LIST_ROW_SZ;
                size_t toRead = sizeof(buf);

                if (toRead > stillBytes)
                    toRead = stillBytes;
                if (toRead == 0U)
                    break;

                size_t got = 0;
                (void)fsReadFile(f, buf, toRead, &got);

                if (got == 0U)
                    break;

                size_t nRows = got / METER_LIST_ROW_SZ;

                for (size_t m = 0; m < nRows; m++)
                {
                    MeterTable_t row;
                    memcpy(&row, buf + m * METER_LIST_ROW_SZ, sizeof(row));
                    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, row.serialNumber);
                    (void)fsDeleteFile((char_t *)path);
                }
                processed += (U32)nRows;

                if (got < toRead)
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

S32 appMeterGetMeterCount(void)
{
    S32 n;

    lockMeterReg();
    n = (S32)s_meterCount;
    unlockMeterReg();
    return n;
}

static S32 enqueueMeterJob(const MeterJobMsg_t *job)
{
    if (s_jobQueue == OS_INVALID_QUEUE || !s_managerRunning)
        return (S32)EN_ERR_CODE_FAILURE;
    if (QUEUE_SUCCESS != zosMsgQueueSend(s_jobQueue, (const char *)job, sizeof(*job), TIME_OUT_10MS))
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    return job->taskId;
}

S32 appMeterAddReadoutTask(const char *request, Callback_t callback)
{
    if (request == NULL || callback == NULL)
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!ZDJson_IsValid(request))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    char serial[20];
    if (!getJsonParamString(request, "METERSERIALNUMBER", serial, sizeof(serial))
        && !ZDJson_GetStringValue(request, "METERSERIALNUMBER", serial, sizeof(serial)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    MeterData_t tmp;
    if (appMeterGetMeterData(serial, &tmp) != SUCCESS)
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

S32 appMeterAddProfileTask(const char *request, Callback_t callback)
{
    if (request == NULL || callback == NULL)
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!ZDJson_IsValid(request))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    char serial[20];
    if (!getJsonParamString(request, "METERSERIALNUMBER", serial, sizeof(serial))
        && !ZDJson_GetStringValue(request, "METERSERIALNUMBER", serial, sizeof(serial)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char d0[40], d1[40];
    if (!getJsonParamString(request, "startDate", d0, sizeof(d0))
        && !ZDJson_GetStringValue(request, "startDate", d0, sizeof(d0)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (!getJsonParamString(request, "endDate", d1, sizeof(d1))
        && !ZDJson_GetStringValue(request, "endDate", d1, sizeof(d1)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char w0[11], w1[11];
    if (!parseProfileWindow(d0, w0) || !parseProfileWindow(d1, w1))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    MeterData_t tmp2;
    if (appMeterGetMeterData(serial, &tmp2) != SUCCESS)
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

S32 appMeterAddObisTask(const char *request, Callback_t callback)
{
    (void)request;
    (void)callback;
    return (S32)EN_ERR_CODE_NOT_IMPLEMENTED;
}

S32 appMeterAddWriteTask(const char *request, Callback_t callback)
{
    (void)request;
    (void)callback;
    return (S32)EN_ERR_CODE_NOT_IMPLEMENTED;
}

ERR_CODE_T appMeterIsTaskDone(S32 taskID)
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
    if (ph == TASK_PHASE_FINISHED)
        return r;
    return EN_ERR_CODE_PENDING;
}

void appMeterTaskIDFree(S32 taskID)
{
    lockTask();
    int ix = taskSlotFindLocked(taskID);
    if (ix < 0)
    {
        unlockTask();
        return;
    }
    if (s_taskSlots[ix].phase != TASK_PHASE_FINISHED)
    {
        unlockTask();
        return;
    }
    s_taskSlots[ix].taskId = 0;
    s_taskSlots[ix].phase = TASK_PHASE_FREE;
    s_taskSlots[ix].result = EN_ERR_CODE_SUCCESS;
    unlockTask();
}

S32 appMeterAddDirective(const char *directive)
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

RETURN_STATUS appMeterDeleteDirective(const char *directiveID)
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

const char *appMeterGetDirective(const char *directiveID)
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
    const char *out = directiveReadFileToScratch((char_t *)path);
    zosReleaseMutex(&s_getDirectiveMux);
    return out;
}

const char *appMeterGetDirectiveByIndex(U32 index)
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
    const char *out = directiveReadFileToScratch((char_t *)path);
    zosReleaseMutex(&s_getDirectiveMux);
    return out;
}

S32 appMeterGetDirectiveCount(void)
{
    lockDirective();
    S32 n = (S32)s_directiveCount;
    unlockDirective();
    return n;
}

/******************************** End Of File *********************************/
