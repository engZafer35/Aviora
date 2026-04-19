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
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"

#include "fs_port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef METER_OPS_TASK_STACK
#define METER_OPS_TASK_STACK  (128)
#endif

#define MAX_DIRECTIVES       (64)
#define MAX_METER_TASKS      (4)
#define METER_OPS_QUEUE_DEPTH (8)
#define DIRECTIVE_INDEX_FILE DIRECTIVE_MAIN_DIR "directive.idx"
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
static MeterCommInterface_t s_meterIf;
static OsTaskId gs_moTaskId = OS_INVALID_TASK_ID;
static OsQueue gs_jobQueue = OS_INVALID_QUEUE;
static bool gs_running = false;

static U32 s_meterCount = 0;

/* Directive index table */
static char s_directiveIds[MAX_DIRECTIVES][MAX_KEY_LENGTH];
static U32 s_directiveCount = 0;

/* Task slots */
static TaskSlot_t s_taskSlots[MAX_METER_TASKS];
static S32 s_nextTaskId = 0;

/* Mutexes */
static ZOsMutex s_meterRegMux;
static ZOsMutex s_directiveMux;
static ZOsMutex s_taskMux;


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
            if (s_nextTaskId < 0)
                s_nextTaskId = 0;
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

static error_t fsWriteWholeFile(const char *path, const void *data, size_t len)
{
    FsFile *f = fsOpenFile(path, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return ERROR_FILE_OPENING_FAILED;
    error_t e = fsWriteFile(f, (void *)data, len);
    fsCloseFile(f);
    return e;
}

static error_t fsReadWholeText(const char *path, char *buf, size_t cap, size_t *outLen)
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
    if (fsGetFileSize((char *)METER_LIST_FILE, &sz) != NO_ERROR || sz == 0)
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

    FsFile *f = fsOpenFile((char *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (f == NULL)
        return FALSE;

    U32 snLeng = strlen(serialKey);

    for (;;)
    {
        size_t got = 0;
        (void)fsReadFile(f, buf, sizeof(buf), &got);
        if (got == 0)
            break;

        size_t nRows = got / METER_LIST_ROW_SZ;
        for (size_t m = 0; m < nRows; m++)
        {
            if (memcmp(buf + m * METER_LIST_ROW_SZ, serialKey, snLeng) == 0)
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
    FsFile *f = fsOpenFile((char *)METER_LIST_FILE, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
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

    FsFile *src = fsOpenFile((char *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (src == NULL)
        return ERROR_FILE_OPENING_FAILED;

    const char *tmpPath = METER_MAIN_DIR "meterList.tmp";
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

    (void)fsDeleteFile((char *)METER_LIST_FILE);
    if (written > 0)
        (void)fsRenameFile(tmpPath, (char *)METER_LIST_FILE);
    else
        (void)fsDeleteFile(tmpPath);

    s_meterCount = written;
    return NO_ERROR;
}

/* ---- Directive index helpers ---- */

static RETURN_STATUS directiveIndexSave(void)
{
    U32 magic = 0x44495258u;
    U8 hdr[sizeof(U32) * 2];

    memcpy(hdr, &magic, sizeof(magic));
    memcpy(hdr + sizeof(U32), &s_directiveCount, sizeof(U32));

    FsFile *f = fsOpenFile((char *)DIRECTIVE_INDEX_FILE,
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return FAILURE;

    error_t e = fsWriteFile(f, hdr, sizeof(hdr));
    if ((NO_ERROR == e) && (s_directiveCount > 0))
        e = fsWriteFile(f, s_directiveIds, (size_t)s_directiveCount * MAX_KEY_LENGTH);
        
    fsCloseFile(f);
    return (e == NO_ERROR) ? SUCCESS : FAILURE;
}

static RETURN_STATUS directiveIndexLoad(void)
{
    U8 hdr[sizeof(U32) * 2];
    s_directiveCount = 0;

    FsFile *f = fsOpenFile((char *)DIRECTIVE_INDEX_FILE, FS_FILE_MODE_READ);
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
    if (data == NULL || len == 0)
        return -1;
    S32 r = s_meterIf.meterCommSend(data, len, timeoutMs);
    return ((U32)r == len) ? 0 : -1;
}

static ERR_CODE_T recvLine(char *line, size_t cap, U32 overallTimeoutMs)
{
    if (line == NULL || cap < 2)
        return EN_ERR_CODE_METER_COMM_LINE_PARAM_ERROR;

    size_t n = 0;

    while (n + 1 < cap)
    {
        U8 b;
        U32 got = 1;
        S32 rr = s_meterIf.meterCommReceive(&b, &got, IEC_BYTE_TIMEOUT_MS);
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
    }
    return EN_ERR_CODE_METER_COMM_LINE_ERROR;
}

/* ---- IEC 62056 readout / profile ---- */

static ERR_CODE_T iecSaveMeterIdFile(S32 taskId, const char *line)
{
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%d_meterID.txt", METER_DATA_OUTPUT_DIR, (int)taskId);
    return (fsWriteWholeFile((char *)path, line, strlen(line)) == NO_ERROR)
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

    FsFile *f = fsOpenFile((char *)pathOut, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
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

    char line[1024];
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

    FsFile *f = fsOpenFile((char *)pathPay, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
    if (f == NULL)
        return EN_ERR_CODE_FAILURE;

    S32 rr;
    U32 got;
    BOOL received = FALSE;
    for (;;)
    {
        got = sizeof(line);
        rr = s_meterIf.meterCommReceive(line, &got, IEC_BYTE_TIMEOUT_MS);
        printf("Received bytes: %d - %d", (int)rr, (int)got);
        if ((rr <= 0) || (got == 0))
        {
            fsCloseFile(f);
            if (FALSE == received)
                return EN_ERR_CODE_METER_COMM_LINE_ERROR;
            else
                return EN_ERR_CODE_SUCCESS;
        }
        /*
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
            */
        fsWriteFile(f, line, got);
        received = TRUE;
        //fsWriteFile(f, (void *)"\n", 1);
    }
    fsCloseFile(f);
    return EN_ERR_CODE_SUCCESS;
}

/* ---- Worker task ---- */

static S32 enqueueMeterJob(const MeterJobMsg_t *job)
{
    if (gs_jobQueue == OS_INVALID_QUEUE || !gs_running)
        return (S32)EN_ERR_CODE_FAILURE;
    if (QUEUE_SUCCESS != zosMsgQueueSend(gs_jobQueue, (const char *)job, sizeof(*job), TIME_OUT_500MS))
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    return job->taskId;
}

static void meterOpsWorkerTask(void *arg)
{
    if (-1 == zosEventGroupWait(gp_systemSetupEventGrp, PROTOCOL_WAIT_DEPENDENCY_FLAGS, INFINITE_DELAY, ZOS_EVENT_WAIT_ALL))
    {
        DEBUG_ERROR("->[E] Display Task: Wait for zosEventGroupWait failed");
        APP_LOG_REC(g_sysLoggerID, "Display Task: Wait for zosEventGroupWait failed");
        appTskMngDelete(&gs_moTaskId);        
    }

    (void)arg;
    MeterJobMsg_t job;
    BOOL meterCommInitialized = FALSE;

    DEBUG_INFO("->[I] %s worker started", METER_OPS_TASK_NAME);
    appTskMngImOK(gs_moTaskId);

    while (gs_running)
    {
        if ((FALSE == meterCommInitialized) && (NULL != s_meterIf.meterCommInit))
        {
            if (SUCCESS == s_meterIf.meterCommInit())
            {
                meterCommInitialized = TRUE;
            }            
        }

        if (0 < zosMsgQueueReceive(gs_jobQueue, (char *)&job, sizeof(job), TIME_OUT_500MS))
        {
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
        appTskMngImOK(gs_moTaskId);
        zosDelayTask(WAIT_1_SEC);
    }

    DEBUG_WARNING("->[W] %s worker stopping", METER_OPS_TASK_NAME);
    zosMsgQueueClose(gs_jobQueue);
    gs_jobQueue = OS_INVALID_QUEUE;
    appTskMngDelete(&gs_moTaskId);
}

/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appMeterOperationsStart(MeterCommInterface_t *meterComm)
{
    if ((meterComm == NULL) || (NULL == meterComm->meterCommInit) || (NULL == meterComm->meterCommSend) || \
        (NULL == meterComm->meterCommReceive) || (NULL == meterComm->meterCommSetBaudrate))
        return FAILURE;

    if (gs_running)
        return SUCCESS;

    s_meterIf = *meterComm;

    if ((zosCreateMutex(&s_meterRegMux) == FALSE) || (zosCreateMutex(&s_directiveMux) == FALSE)
        || (zosCreateMutex(&s_taskMux) == FALSE))
        return FAILURE;

    lockMeterReg();
    meterListRefreshCount();
    unlockMeterReg();

    lockDirective();
    directiveIndexLoad();
    unlockDirective();

    for (U32 i = 0; i < MAX_METER_TASKS; i++)
    {
        s_taskSlots[i].taskId = 0;
        s_taskSlots[i].phase = TASK_PHASE_FREE;
        s_taskSlots[i].result = EN_ERR_CODE_SUCCESS;
    }

    gs_running = true;

    gs_jobQueue = zosMsgQueueCreate(QUEUE_NAME("MeterOpsQ"), METER_OPS_QUEUE_DEPTH, sizeof(MeterJobMsg_t));
    if (gs_jobQueue == OS_INVALID_QUEUE)
    {
        gs_running = false;
        return FAILURE;
    }

    ZOsTaskParameters taskParam;
    taskParam.priority = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = METER_OPS_TASK_STACK*5;

    gs_moTaskId = appTskMngCreate(METER_OPS_TASK_NAME, meterOpsWorkerTask, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_moTaskId)
    {
        zosMsgQueueClose(gs_jobQueue);
        gs_jobQueue = OS_INVALID_QUEUE;
        gs_running = false;
        return FAILURE;
    }

    DEBUG_INFO("->[I] Meter operations service started");
    return SUCCESS;
}

/* ---- Meter registration ---- */

void initMeterTest(void)
{
    zosCreateMutex(&s_meterRegMux);
    zosCreateMutex(&s_directiveMux);
    zosCreateMutex(&s_taskMux);

    meterListRefreshCount();
    directiveIndexLoad();
}
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
        return FAILURE;
    }

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, meterData->serialNumber);
    error_t e = fsWriteWholeFile((char *)path, meterData, sizeof(MeterData_t));
    if (e != NO_ERROR)
    {
        unlockMeterReg();
        return FAILURE;
    }

    e = meterListAppendRow(&row);
    if (e != NO_ERROR)
    {
        (void)fsDeleteFile((char *)path);
        unlockMeterReg();
        return FAILURE;
    }
    s_meterCount++;

    DEBUG_INFO("->[I] Meter %s added", meterData->serialNumber);
    APP_LOG_REC(g_sysLoggerID, "Meter added");

    unlockMeterReg();
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsGetMeterData(const char *serialNumber, MeterData_t *meterDataOut)
{
    char path[MAX_PATH_LEN];

    if ((NULL == serialNumber) || (serialNumber[0] == '\0') || (NULL == meterDataOut))
        return FAILURE;

    snprintf(path, sizeof(path), "%s%.16s", METER_REG_DIR, serialNumber);

    FsFile *f = fsOpenFile((char *)path, FS_FILE_MODE_READ);
    if (f == NULL)
        return FAILURE;

    size_t got = 0;
    if ((NO_ERROR != fsReadFile(f, meterDataOut, sizeof(MeterData_t), &got)) || (got != sizeof(MeterData_t)))
    {
        fsCloseFile(f);
        return FAILURE;
    }

    fsCloseFile(f);
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsGetMeterDataByIndex(U32 index, MeterData_t *meterDataOut)
{
    lockMeterReg();

    if (index >= s_meterCount)
    {
        unlockMeterReg();
        return FAILURE;
    }

    FsFile *f = fsOpenFile((char *)METER_LIST_FILE, FS_FILE_MODE_READ);
    if (NULL == f)
    {
        unlockMeterReg();
        return FAILURE;
    }

    if (NO_ERROR != fsSeekFile(f, (int)(index * METER_LIST_ROW_SZ), FS_SEEK_SET))
    {
        fsCloseFile(f);
        unlockMeterReg();
        return FAILURE;
    }

    MeterTable_t row;
    size_t got = 0;
    if ((NO_ERROR != fsReadFile(f, &row, sizeof(row), &got)) || (got < sizeof(row)))
    {
        fsCloseFile(f);
        unlockMeterReg();
        return FAILURE;
    }
    fsCloseFile(f);
    unlockMeterReg();

    return appMeterOperationsGetMeterData(row.serialNumber, meterDataOut);
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
    (void)fsDeleteFile((char *)path);

    DEBUG_INFO("->[I] Meter %s deleted", serialNumber);
    APP_LOG_REC(g_sysLoggerID, "Meter deleted");

    unlockMeterReg();
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsDeleteAllMeters(void)
{
    char path[MAX_PATH_LEN];
    U8 buf[METER_LIST_ROW_SZ * 32]; //32 is temp value. chunk size

    lockMeterReg();

    if (s_meterCount > 0)
    {
        FsFile *f = fsOpenFile((char *)METER_LIST_FILE, FS_FILE_MODE_READ);
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
                    (void)fsDeleteFile((char *)path);
                }
                if (got < sizeof(buf))
                    break;
            }
            fsCloseFile(f);
        }
    }

    (void)fsDeleteFile((char *)METER_LIST_FILE);
    s_meterCount = 0;

    DEBUG_INFO("->[I] All meters deleted");
    APP_LOG_REC(g_sysLoggerID, "All meters deleted");

    unlockMeterReg();
    return SUCCESS;
}

U32 appMeterOperationsGetMeterCount(void)
{
    lockMeterReg();
    U32 n = s_meterCount;
    unlockMeterReg();
    return n;
}

/* ---- Task API ---- */

S32 appMeterOperationsAddReadoutTask(const char *request, Callback_t callback)
{
    if ((request == NULL) || (callback == NULL))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (FALSE == ZDJson_IsValid(request))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    char serial[20];
    if (FALSE == getJsonParamString(request, "METERSERIALNUMBER", serial, sizeof(serial)))
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;

    MeterData_t tmp;
    if (SUCCESS != appMeterOperationsGetMeterData(serial, &tmp))
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

    DEBUG_INFO("->[I] Readout task %d added", tid);
    APP_LOG_REC(g_sysLoggerID, "Readout task added");

    return tid;
}

S32 appMeterOperationsAddProfileTask(const char *request, Callback_t callback)
{
    if (request == NULL || callback == NULL)
        return (S32)EN_ERR_CODE_METER_PARAM_ERROR;
    if (FALSE == ZDJson_IsValid(request))
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

    DEBUG_INFO("->[I] Profile task %d added", tid);
    APP_LOG_REC(g_sysLoggerID, "Profile task added");

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
    ERR_CODE_T retVal = EN_ERR_CODE_FAILURE;
    if (taskID <= 0)
        return EN_ERR_CODE_FAILURE;
    
    lockTask();
    int ix = taskSlotFindLocked(taskID);
    if (ix >= 0)
    {
        retVal = (TASK_PHASE_FINISHED == s_taskSlots[ix].phase) ? s_taskSlots[ix].result : EN_ERR_CODE_PENDING;
    }
    unlockTask();
    return retVal;
}

ERR_CODE_T appMeterOperationsTaskIDFree(S32 taskID)
{
    ERR_CODE_T retVal = EN_ERR_CODE_FAILURE;
    lockTask();

    int ix = taskSlotFindLocked(taskID);
    if ((ix >= 0) && (TASK_PHASE_FINISHED == s_taskSlots[ix].phase))
    {
        s_taskSlots[ix].taskId = 0;
        s_taskSlots[ix].phase = TASK_PHASE_FREE;
        s_taskSlots[ix].result = EN_ERR_CODE_SUCCESS;
        retVal = EN_ERR_CODE_SUCCESS;

        DEBUG_INFO("->[I] Task %d freed", taskID);
        APP_LOG_REC(g_sysLoggerID, "Task freed");
    }
    
    unlockTask();
    return retVal;
}

/* ---- Directive management ---- */

S32 appMeterOperationsAddDirective(const char *directive)
{
    if ((NULL == directive) || (FALSE == ZDJson_IsValid(directive)))
    {
        DEBUG_INFO("->[E] Directive add failed: invalid JSON");
        APP_LOG_REC(g_sysLoggerID, "Directive add failed: invalid JSON");
        return (S32)EN_ERR_CODE_DIRECTIVE_PARAM_ERROR;
    }

    char id[MAX_KEY_LENGTH];
    if (FALSE == ZDJson_GetStringValue(directive, "id", id, sizeof(id)))
    {
        DEBUG_INFO("->[E] Directive add failed ID not found");
        APP_LOG_REC(g_sysLoggerID, "Directive add failed ID not found");
        return (S32)EN_ERR_CODE_DIRECTIVE_PARAM_ERROR;
    }

    lockDirective();

    S32 idx = directiveIndexFind(id);

    if ((idx < 0) && (s_directiveCount >= MAX_DIRECTIVES))
    {
        unlockDirective();
        DEBUG_INFO("->[E] Directive add failed: max directives reached");
        APP_LOG_REC(g_sysLoggerID, "Directive add failed: max directives reached");
        return (S32)EN_ERR_CODE_NO_RESOURCES;
    }

    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%s.json", DIRECTIVE_MAIN_DIR, id);
    path[sizeof(path) - 1] = '\0';

    if (NO_ERROR != fsWriteWholeFile((char *)path, directive, strlen(directive)))
    {
        DEBUG_INFO("->[E] Directive add failed: Write failed");
        APP_LOG_REC(g_sysLoggerID, "Directive add failed: Write failed");
        unlockDirective();
        return (S32)EN_ERR_CODE_FAILURE;
    }

    BOOL isNew = (idx < 0) ? TRUE : FALSE;
    if (isNew)
    {
        strncpy(s_directiveIds[s_directiveCount], id, MAX_KEY_LENGTH - 1);
        s_directiveIds[s_directiveCount][MAX_KEY_LENGTH - 1] = '\0';
        idx = (S32)s_directiveCount;
        s_directiveCount++;
    }

    if (SUCCESS != directiveIndexSave())
    {
        if (isNew) { s_directiveCount--; }
        (void)fsDeleteFile((char *)path);

        DEBUG_INFO("->[E] Directive add failed: Index save failed");
        APP_LOG_REC(g_sysLoggerID, "Directive add failed: Index save failed");

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
            snprintf(path, sizeof(path), "%s%s.json", DIRECTIVE_MAIN_DIR, s_directiveIds[i]);
            path[sizeof(path) - 1] = '\0';
            (void)fsDeleteFile((char *)path);
        }
        s_directiveCount = 0;
        (void)fsDeleteFile((char *)DIRECTIVE_INDEX_FILE);

        DEBUG_INFO("->[I] All directives deleted");
        APP_LOG_REC(g_sysLoggerID, "All directives deleted");

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
    snprintf(path, sizeof(path), "%s%s.json", DIRECTIVE_MAIN_DIR, directiveID);
    path[sizeof(path) - 1] = '\0';
    (void)fsDeleteFile((char *)path);

    for (U32 j = (U32)ix + 1; j < s_directiveCount; j++)
        memcpy(s_directiveIds[j - 1], s_directiveIds[j], MAX_KEY_LENGTH);
    s_directiveCount--;

    // we cannot check the return valud, because files are deleted and index set to 0
    directiveIndexSave(); 

    DEBUG_INFO("->[I] Directive %s deleted", directiveID);
    APP_LOG_REC(g_sysLoggerID, "Directive deleted");

    unlockDirective();
    return SUCCESS;
}

RETURN_STATUS appMeterOperationsGetDirective(const char *directiveID, char *directiveOut, size_t directiveOutSz)
{
    if (directiveID == NULL || directiveOut == NULL || directiveOutSz == 0)
        return FAILURE;

    lockDirective();
    if (directiveIndexFind(directiveID) < 0)
    {
        unlockDirective();
        return FAILURE;
    }
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s%s.json", DIRECTIVE_MAIN_DIR, directiveID);
    path[sizeof(path) - 1] = '\0';
    unlockDirective();

    size_t got = 0;
    error_t e = fsReadWholeText((char *)path, directiveOut, directiveOutSz, &got);
    return (e == NO_ERROR) ? SUCCESS : FAILURE;
}

RETURN_STATUS appMeterOperationsGetDirectiveByIndex(U32 index, char *directiveOut, size_t directiveOutSz)
{
    if (directiveOut == NULL || directiveOutSz == 0)
        return FAILURE;

    char path[MAX_PATH_LEN];

    lockDirective();
    if (index >= s_directiveCount)
    {
        unlockDirective();
        return FAILURE;
    }
    snprintf(path, sizeof(path), "%s%s.json", DIRECTIVE_MAIN_DIR, s_directiveIds[index]);
    path[sizeof(path) - 1] = '\0';
    unlockDirective();

    size_t got = 0;
    error_t e = fsReadWholeText((char *)path, directiveOut, directiveOutSz, &got);
    return (e == NO_ERROR) ? SUCCESS : FAILURE;
}

U32 appMeterOperationsGetDirectiveCount(void)
{
    lockDirective();
    U32 n = s_directiveCount;
    unlockDirective();
    return n;
}

/******************************** End Of File *********************************/
