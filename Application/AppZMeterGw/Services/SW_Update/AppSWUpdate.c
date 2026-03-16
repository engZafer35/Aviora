/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 14 Mar 2026 - 18:21:51
* #File Name    : AppSWUpdate.c
*******************************************************************************/
/******************************************************************************
* This module implements a software update service for the application. 
* It allows the application to perform software updates by downloading
* update files from an FTP server. The service is designed to run as a separate task, 
* and it communicates the update status through a data bus.
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppSWUpdate.h"
#include "AppTaskManager.h"
#include "AppDataBus.h"
#include "AppInternalMsgFrame.h"
#include "Project_Conf.h"
#include "ZDebug.h"

#include "core/net.h"
#include "ftp/ftp_client.h"
#include "fs_port.h"

#include <string.h>
/****************************** MACRO DEFINITIONS *****************************/
#define SW_UPDATE_MAX_HOST          (64)
#define SW_UPDATE_MAX_USER          (32)
#define SW_UPDATE_MAX_PASS          (32)
#define SW_UPDATE_MAX_PATH          (128)
#define SW_UPDATE_FTP_READ_SIZE     (1024)

#define SW_UPDATE_TASK_NAME      "SW_UPDATE"
#define SW_UPDATE_TASK_STACK     2048
#define SW_UPDATE_TASK_PRIORITY  osPriorityNormal

/******************************* TYPE DEFINITIONS *****************************/
typedef struct {
    char serverIp[SW_UPDATE_MAX_HOST];    
    char username[SW_UPDATE_MAX_USER];
    char password[SW_UPDATE_MAX_PASS];
    char remoteFile[SW_UPDATE_MAX_PATH];
    char localFile[SW_UPDATE_MAX_PATH];
    OsTaskId taskId;
    U16  serverPort;
} AppSwUpdateContext;

/********************************** VARIABLES *********************************/
static AppSwUpdateContext gs_swUpdateCtx;
static S32 gs_swUpdateDbusID;

/***************************** STATIC FUNCTIONS  ******************************/
static void appSwUpdatePublishStatus(SW_UPDATE_RESULT result)
{
    DBUS_PACKET packet;

    packet.packetID     = 0;
    packet.topic        = EN_DBUS_TOPIC_DEVICE;
    packet.pri          = EN_PRIORITY_LOW;
    packet.retainFlag   = FALSE;
    
    packet.payload.data[0] = (U8)result;
    packet.payload.data[1] = '\0';
    packet.payload.dataLeng = 2;

    appDBusPublish(gs_swUpdateDbusID, &packet);
}

static void appSwUpdateTask(void *arg)
{
    AppSwUpdateContext *ctx = (AppSwUpdateContext *)arg;
    FtpClientContext ftpContext;
    IpAddr ipAddr;
    FsFile *outputFile = NULL;
    error_t error;
    U32 bytesRead;
    char_t readBuffer[SW_UPDATE_FTP_READ_SIZE];

    appTskMngImOK(ctx->taskId);
    if ((ctx == NULL) || (ctx->serverIp[0] == '\0') || (ctx->remoteFile[0] == '\0') || (ctx->localFile[0] == '\0'))
    {
        DEBUG_ERROR("Invalid parameters for SW update task");
        appLogRec(g_sysLoggerID, "Invalid parameters for SW update task");

        appSwUpdatePublishStatus(EN_SW_UPDATE_RESULT_INVALID_PARAM);
        
        if (gs_swUpdateDbusID > 0)
        {
            appDBusUnregister(gs_swUpdateDbusID);
            gs_swUpdateDbusID = -1;
        }

        //ctx->taskId, task id is clear in task delete function, no need to set it again
        appTskMngDelete(&ctx->taskId);        

        return;
    }

    ftpClientInit(&ftpContext);
    do
    {
        appTskMngImOK(ctx->taskId);
        error = gethostbyname(NULL, ctx->serverIp, &ipAddr, 0);
        if (error)
        {
            break;
        }

        ftpClientSetTimeout(&ftpContext, 20000);

        error = ftpClientConnect(&ftpContext, &ipAddr, ctx->serverPort, FTP_MODE_PLAINTEXT | FTP_MODE_PASSIVE);
        if (error) { break; }

        error = ftpClientLogin(&ftpContext, ctx->username, ctx->password);
        if (error) { break; }

        error = ftpClientOpenFile(&ftpContext, ctx->remoteFile, FTP_FILE_MODE_READ | FTP_FILE_MODE_BINARY);
        if (error) { break; }

        outputFile = fsOpenFile(ctx->localFile, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
        if (outputFile == NULL)
        {
            ftpClientCloseFile(&ftpContext);
            break;
        }
        
        while (1)
        {
            appTskMngImOK(ctx->taskId);
            
            error = ftpClientReadFile(&ftpContext, readBuffer, sizeof(readBuffer), &bytesRead, 0);
            if (error == ERROR_END_OF_STREAM)
            {
                /* done */
                error = NO_ERROR;
                break;
            }

            if (error)
            {
                break;
            }

            if (bytesRead > 0)
            {
                error = fsWriteFile(outputFile, readBuffer, bytesRead);
                if (error)
                {
                    break;
                }
            }
        }

        if (outputFile)
        {
            fsCloseFile(outputFile);
            outputFile = NULL;
        }

        ftpClientCloseFile(&ftpContext);
        ftpClientDisconnect(&ftpContext);

        if (!error)
        {
            appSwUpdatePublishStatus(EN_SW_UPDATE_RESULT_SUCCESS);
        }
    } while (0);

    if (error)
    {
        appSwUpdatePublishStatus(EN_SW_UPDATE_RESULT_FAILED);
        break;
    }

    if (outputFile != NULL)
    {
        fsCloseFile(outputFile);
    }

    ftpClientDeinit(&ftpContext);

    if (gs_swUpdateDbusID >= 0)
    {
        appDBusUnregister(gs_swUpdateDbusID);
        gs_swUpdateDbusID = -1;
    }

    //ctx->taskId, task id is clear in task delete function, no need to set it again.
    appTskMngDelete(&ctx->taskId);   
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS AppSwUpdateInit(const char *serverIp,
                              U16 serverPort,
                              const char *username,
                              const char *password,
                              const char *remoteFilePath,
                              const char *localFilePath)
{
    if (OS_INVALID_TASK_ID != gs_swUpdateCtx.taskId)
    {
        // SW update task is already running
        return FAILURE;
    }

    if ((serverIp == NULL) || (username == NULL) || (password == NULL) ||
        (remoteFilePath == NULL) || (localFilePath == NULL) || (serverPort == 0))
    {
        return FAILURE;
    }

    if ((strlen(serverIp) >= SW_UPDATE_MAX_HOST) ||
        (strlen(username) >= SW_UPDATE_MAX_USER) ||
        (strlen(password) >= SW_UPDATE_MAX_PASS) ||
        (strlen(remoteFilePath) >= SW_UPDATE_MAX_PATH) ||
        (strlen(localFilePath) >= SW_UPDATE_MAX_PATH))
    {
        return FAILURE;
    }

    memset(&gs_swUpdateCtx, 0, sizeof(gs_swUpdateCtx));
    strncpy(gs_swUpdateCtx.serverIp, serverIp, SW_UPDATE_MAX_HOST - 1);
    gs_swUpdateCtx.serverPort = serverPort;
    strncpy(gs_swUpdateCtx.username, username, SW_UPDATE_MAX_USER - 1);
    strncpy(gs_swUpdateCtx.password, password, SW_UPDATE_MAX_PASS - 1);
    strncpy(gs_swUpdateCtx.remoteFile, remoteFilePath, SW_UPDATE_MAX_PATH - 1);
    strncpy(gs_swUpdateCtx.localFile, localFilePath, SW_UPDATE_MAX_PATH - 1);

    ZOsTaskParameters taskParam;
    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = SW_UPDATE_TASK_STACK;

    if (SUCCESS != appDBusRegister(EN_DBUS_TOPIC_GSM | EN_DBUS_TOPIC_ETH | EN_DBUS_TOPIC_DEVICE, &gs_swUpdateDbusID))
    {
        return FAILURE;
    }

    gs_swUpdateCtx.taskId = appTskMngCreate(SW_UPDATE_TASK_NAME, (OsTaskCode)appSwUpdateTask, &gs_swUpdateCtx, &taskParam);
    if (gs_swUpdateCtx.taskId == OS_INVALID_TASK_ID)
    {
        appDBusUnregister(gs_swUpdateDbusID);
        gs_swUpdateDbusID = -1;
        return FAILURE;
    }

    return SUCCESS;
}

/******************************** End Of File *********************************/
