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
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppLogRecorder.h"
#include "AppTaskManager.h"
#include "AppTimeService.h"
#include "AppGlobalVariables.h"
#include "Midd_OSPort.h"


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
    logRecConf_t srvIFace;
    S32 loggerID;
    char serviceName[SERVICE_NAME_MAX];
    OsQueue queue;
    OsTaskId writerTask;
    BOOL taskRunning;
    FsFile *logFile;
    U32 currentFileSize;
    char currentLogFile[LOG_FILE_PATH_MAX];
} LoggerService;

/********************************** VARIABLES *********************************/
static LoggerService services[MAX_SERVICE_NUMBER];

/***************************** STATIC FUNCTIONS  ******************************/
static void getCurrentDate_(char *dest, U32 destSize)
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
    getCurrentDate_(dateStr, sizeof(dateStr));

    if (svc->logFile)
    {
        svc->srvIFace.closeFunc(svc->logFile);
        svc->logFile = NULL;
    }

    snprintf(svc->currentLogFile, sizeof(svc->currentLogFile), "%s/%s/%s-%02d%02d%02d.log",
                                                                svc->srvIFace.logPath,
                                                                svc->serviceName, 
                                                                dateStr,
                                                                tmValue.tm_hour, tmValue.tm_min, tmValue.tm_sec);

    svc->logFile = svc->srvIFace.openFunc(svc->currentLogFile, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
    if (NULL == svc->logFile)
    {
        DEBUG_ERROR("->[E] %s failed to open log file %s", svc->serviceName, svc->currentLogFile);
        return FAILURE;
    }

    svc->currentFileSize = 0;
    DEBUG_INFO("->[I] %s opened log file %s", svc->serviceName, svc->currentLogFile);

    return SUCCESS;
}

static void loggerWriterTask(void *arg)
{
    LoggerService *svc = (LoggerService *)arg;    

    DEBUG_INFO("->[I] %s Logger writer task waiting for eventsGroup", svc->serviceName);
    if (-1 == zosEventGroupWait(gp_systemSetupEventGrp, LOGGER_WAIT_DEPENDENCY_FLAGS, INFINITE_DELAY, ZOS_EVENT_WAIT_ALL))
    {
        DEBUG_ERROR("->[E] %s failed to wait for logger dependencies", svc->serviceName);
        //don't need to check queue if it is valid or not, because if task is running, queue should be valid.
        zosMsgQueueClose(svc->queue);
        svc->queue = OS_INVALID_QUEUE;  

        svc->serviceName[0] = '\0';
        svc->loggerID = -1;
        svc->currentFileSize = 0;
        svc->currentLogFile[0] = '\0';

        //ctx->writerTask, task id is clear in task delete function, no need to set it again.
        appTskMngDelete(&svc->writerTask);
    }
    
    LoggerQueueItem item;

    DEBUG_INFO("->[I] %s Logger writer task started", svc->serviceName);
    appTskMngImOK(svc->writerTask);

    zosEventGroupSet(gp_systemSetupEventGrp, LOGGER_SERVICE_READY_FLAG);

    while (svc->taskRunning)
    {
        if (QUEUE_SUCCESS == zosMsgQueueReceive(svc->queue, (char *)&item, sizeof(item), TIME_OUT_500MS))
        {            
            if (NULL == svc->logFile)
            {
                openNewLogFile(svc);
            }
            if (NULL != svc->logFile)
            {
                if (svc->currentFileSize >= svc->srvIFace.fileSize)
                {
                    openNewLogFile(svc);
                }
                char timeStr[16];
                char dateStr[16];
                getCurrentTime(timeStr, sizeof(timeStr));
                getCurrentDate_(dateStr, sizeof(dateStr));

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

        appTskMngImOK(svc->writerTask);
    }

    DEBUG_WARNING("->[W] %s Logger writer task stopped", svc->serviceName);
    if (svc->logFile != NULL)
    {
        svc->srvIFace.closeFunc(svc->logFile);
        svc->logFile = NULL;
    }
    
    //don't need to check queue if it is valid or not, because if task is running, queue should be valid.
    zosMsgQueueClose(svc->queue);
    svc->queue = OS_INVALID_QUEUE;  

    svc->serviceName[0] = '\0';
    svc->loggerID = -1;
    svc->currentFileSize = 0;
    svc->currentLogFile[0] = '\0';

    //ctx->writerTask, task id is clear in task delete function, no need to set it again.
    appTskMngDelete(&svc->writerTask);
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appLogRecInit(void)
{
    for (U32 i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        services[i].loggerID = -1;
        services[i].serviceName[0] = '\0';
        services[i].queue = OS_INVALID_QUEUE;
        services[i].taskRunning = FALSE;
        services[i].logFile = NULL;
        services[i].currentFileSize = 0;
        services[i].currentLogFile[0] = '\0';        
    }
    return SUCCESS;
}

RETURN_STATUS appLogRecRegister(logRecConf_t *logger, const char *srvName, S32 *loggerID)
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

            services[i].taskRunning = TRUE;
            services[i].logFile = NULL;
            services[i].currentFileSize = 0;
            services[i].currentLogFile[0] = '\0';

            ZOsTaskParameters taskParam;
            taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
            taskParam.stackSize = 2048;

            services[i].writerTask = appTskMngCreate(services[i].serviceName, loggerWriterTask, &services[i], &taskParam);
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
            DEBUG_INFO("->[I] %s registered with logger ID %d", srvName, services[i].loggerID);
            return SUCCESS;
        }
    }

    DEBUG_ERROR("->[E] %s registration failed", srvName);
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

    /*
    * task will be stopped in loggerWriterTask when taskRunning is set to false,
    * and task will be deleted in loggerWriterTask before exiting.
    */
    svc->taskRunning = FALSE;

    DEBUG_WARNING("->[W] %s unregistered with logger ID %d", srvName, loggerID);
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

S32 appLogRecRead(S32 loggerID, char *str, U32 size)
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

    size_t outLen = 0;
    svc->srvIFace.readFunc(svc->logFile, str, size, &outLen);
    return outLen;
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

/******************************** End Of File *********************************/
