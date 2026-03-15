/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
* #Date         : 15 Mar 2026 - 12:46:00
* #File Name    : AppLogRecorder.c
*******************************************************************************/
/******************************************************************************
* This module implements a log recorder service for application services. It allows
* services to register a logger interface, and then log messages through that
* interface. The log recorder manages log files, including rotation based on file
* size and date, and can also send log files through specified methods (e.g., FTP).
*******************************************************************************/
/********************************* INCLUDES ***********************************/
#include "AppLogRecorder.h"
#include "Midd_OSPort.h"
#include "AppTimeService.h"
#include "fs_port.h"
#include "fs_port_posix.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/****************************** MACRO DEFINITIONS *****************************/
#define LOG_FILE_PATH_MAX   (128)
#define SERVICE_NAME_MAX    (16)

/******************************* TYPE DEFINITIONS *****************************/
typedef struct
{
    char text[LOGGER_LOG_MAX_LENGTH];
} LoggerQueueItem;

typedef struct
{
    LogRecInterface srvIFace;
    S32 loggerID;
    char serviceName[SERVICE_NAME_MAX];
    OsQueue queue;
    OsTaskId writerTask;
    bool taskRunning;
    FsFile *logFile;
    U32 currentFileSize;
    char currentLogFile[LOG_FILE_PATH_MAX];
} LoggerService;

/********************************** VARIABLES *********************************/
static LoggerService services[MAX_SERVICE_NUMBER];

/***************************** STATIC FUNCTIONS  ******************************/
static void getCurrentDate(char *dest, U32 destSize)
{
    struct tm tmValue;
    appTimeServiceGetTime(&tmValue);
    snprintf(dest, destSize, "%04d%02d%02d", tmValue.tm_year + 1900, tmValue.tm_mon + 1, tmValue.tm_mday);
    dest[destSize - 1] = '\0';
}

static void getCurrentTime(char *dest, U32 destSize)
{
    struct tm tmValue;

    appTimeServiceGetTime(&tmValue);
    snprintf(dest, destSize, "%02d:%02d:%02d", tmValue.tm_hour, tmValue.tm_min, tmValue.tm_sec);
    dest[destSize - 1] = '\0';
}

static RETURN_STATUS openNewLogFile(LoggerService *svc)
{
    struct tm tmValue;
    char dateStr[16];
    
    appTimeServiceGetTime(&tmValue);
    getCurrentDate(dateStr, sizeof(dateStr));

    if (svc->logFile)
    {
        svc->srvIFace.closeFunc(svc->logFile);
        svc->logFile = NULL;
    }

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

    snprintf(svc->currentLogFile, sizeof(svc->currentLogFile), "%s%c%s_%s-%02d%02d%02d.log",
                                                                svc->srvIFace.logPath, PATH_SEP, 
                                                                svc->serviceName, 
                                                                svc->currentDate, 
                                                                tmValue.tm_hour, tmValue.tm_min, tmValue.tm_sec);

    svc->logFile = svc->srvIFace.openFunc(svc->currentLogFile, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
    if (NULL == svc->logFile)
    {
        return FAILURE;
    }

    svc->currentFileSize = 0;

    return SUCCESS;
}

static void loggerWriterTask(void *arg)
{
    LoggerService *svc = (LoggerService *)arg;
    LoggerQueueItem item;

    while (svc->taskRunning)
    {
        if (QUEUE_SUCCESS == zosMsgQueueReceive(svc->queue, (char *)&item, sizeof(item), TIME_OUT_10MS))
        {
            if (NULL == svc->logFile)
            {
                openNewLogFile(svc);
            }
            if (NULL != svc->logFile)
            {
                if (svc->currentFileSize >= LOGGER_MAX_FILE_SIZE)
                {
                    openNewLogFile(svc);
                }
                char timeStr[16];
                char dateStr[16];
                getCurrentTime(timeStr, sizeof(timeStr));
                getCurrentDate(dateStr, sizeof(dateStr));

                char line[LOGGER_LOG_MAX_LENGTH + 64];
                int lineLen = snprintf(line, sizeof(line), "%s %s - %s\n", dateStr, timeStr, item.text);
                if (lineLen > 0)
                {
                    if (SUCCESS == svc->srvIFace.writeFunc(svc->logFile, line, (size_t)lineLen))
                    {
                        svc->currentFileSize += (U32)lineLen;
                    }
                }
            }
        }
    }

    if (svc->logFile != NULL)
    {
        svc->srvIFace.closeFunc(svc->logFile);
        svc->logFile = NULL;
    }
    if (svc->queue != OS_INVALID_QUEUE)
    {
        zosMsgQueueClose(svc->queue);
        svc->queue = OS_INVALID_QUEUE;
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appLogRecInit(void)
{
    for (U32 i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        services[i].loggerID = -1;
        services[i].serviceName[0] = '\0';
        services[i].queue = OS_INVALID_QUEUE;
        services[i].taskRunning = false;
        services[i].logFile = NULL;
        services[i].currentFileSize = 0;
        services[i].currentDate[0] = '\0';
        services[i].dailyNewIndex = 0;
        services[i].historyCount = 0;
        services[i].historyStart = 0;
        services[i].currentLogFile[0] = '\0';
        memset(services[i].fileHistory, 0, sizeof(services[i].fileHistory));
    }
    return SUCCESS;
}

RETURN_STATUS appLogRecRegister(LogRecInterface *logger, const char *srvName, S32 *loggerID)
{
    if ((logger == NULL) || (srvName == NULL) || (loggerID == NULL))
    {
        return FAILURE;
    }

    for (U32 i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        if (-1 == services[i].loggerID)
        {
            services[i].srvIFace = *logger;
            strncpy(services[i].serviceName, srvName, SERVICE_NAME_MAX - 1);
            services[i].serviceName[SERVICE_NAME_MAX - 1] = '\0';
            services[i].loggerID = (S32)i;
            
            services[i].queue = zosMsgQueueCreate(QUEUE_NAME(services[i].serviceName), LOGGER_QUEUE_SIZE, sizeof(LoggerQueueItem));
            if (services[i].queue == OS_INVALID_QUEUE)
            {
                services[i].loggerID = -1;
                services[i].serviceName[0] = '\0';
                return FAILURE;
            }

            services[i].taskRunning = true;
            services[i].logFile = NULL;
            services[i].currentFileSize = 0;
            services[i].currentDate[0]  = '\0';
            services[i].dailyNewIndex   = 0;
            services[i].historyCount    = 0;
            services[i].historyStart    = 0;
            services[i].currentLogFile[0] = '\0';
            memset(services[i].fileHistory, 0, sizeof(services[i].fileHistory));

            ZOsTaskParameters params = ZOS_TASK_DEFAULT_PARAMS;
            services[i].writerTask = zosCreateTask(services[i].serviceName, loggerWriterTask, &services[i], &params);
            if (OS_INVALID_TASK_ID == services[i].writerTask)
            {
                zosMsgQueueClose(services[i].queue);
                services[i].queue = OS_INVALID_QUEUE;
                services[i].loggerID = -1;
                services[i].serviceName[0] = '\0';
                return FAILURE;
            }

            //dont need to create mutex here, because log file operations are done in the writer task, and there is no concurrent access to log file.
            // If in the future, if log file operations are needed to be done in other tasks, then we can create mutex here and use it in those tasks
            // and writer task to protect log file operations.
            //zosCreateMutex(&services[i].srvIFace.logMutex);

            *loggerID = services[i].loggerID;
            return SUCCESS;
        }
    }

    return FAILURE;
}

RETURN_STATUS appLogRecUnregister(const char *srvName, S32 loggerID)
{
    if ((loggerID < 0) || (loggerID >= MAX_SERVICE_NUMBER) || (srvName == NULL))
    {
        return FAILURE;
    }

    LoggerService *svc = &services[loggerID];
    if (0 != strcmp(svc->serviceName, srvName))
    {
        return FAILURE;
    }

    svc->taskRunning = false;
    if (svc->queue != OS_INVALID_QUEUE)
    {
        zosMsgQueueClose(svc->queue);
        svc->queue = OS_INVALID_QUEUE;
    }
    if (svc->logFile != NULL)
    {
        fsCloseFile(svc->logFile);
        svc->logFile = NULL;
    }

    svc->serviceName[0] = '\0';
    svc->loggerID = -1;
    svc->currentFileSize = 0;
    svc->currentDate[0] = '\0';
    svc->dailyNewIndex = 0;
    svc->historyCount = 0;
    svc->historyStart = 0;
    svc->currentLogFile[0] = '\0';

    return SUCCESS;
}

RETURN_STATUS appLogRec(S32 loggerID, const char *logStr)
{    
    if ((loggerID < 0) || (loggerID >= MAX_SERVICE_NUMBER) || (logStr == NULL))
    {
        return FAILURE;
    }

    LoggerService *svc = &services[loggerID];
    if ((svc->loggerID != loggerID) || (OS_INVALID_QUEUE == svc->queue) || !svc->taskRunning)
    {
        return FAILURE;
    }

    LoggerQueueItem item;
    strncpy(item.text, logStr, LOGGER_LOG_MAX_LENGTH - 1);
    item.text[LOGGER_LOG_MAX_LENGTH - 1] = '\0';

    if (QUEUE_SUCCESS != zosMsgQueueSend(svc->queue, (const char *)&item, sizeof(item), TIME_OUT_10MS))
    {
        return FAILURE;
    }

    return SUCCESS;
}

S32 appLogRecRead(S32 loggerID, const char *str, U32 size)
{
    if ((loggerID < 0) || (loggerID >= MAX_SERVICE_NUMBER) || (str == NULL))
    {
        return -1;
    }

    LoggerService *svc = &services[loggerID];
    if ((-1 == svc->loggerID) || (NULL == svc->srvIFace.readFunc))
    {
        return -1;
    }

    return svc->srvIFace.readFunc(str, size);
}

S32 appLogRecGetLoggerID(const char *srvName)
{
    if (NULL == srvName)
    {
        return -1;
    }

    for (U32 i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        if ((services[i].loggerID != -1) && (0 == strcmp(services[i].serviceName, srvName)))
        {
            return services[i].loggerID;
        }
    }

    return -1;
}

RETURN_STATUS appLogRecSendLog(S32 loggerID, EN_SRV_LOG_SEND sendType, U32 fileNum)
{
    (void)loggerID;
    (void)sendType;
    (void)fileNum;
    return SUCCESS;
}

/******************************** End Of File *********************************/