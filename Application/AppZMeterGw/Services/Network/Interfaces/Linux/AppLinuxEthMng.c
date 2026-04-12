/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Cry For The Moon
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 14:21:51
* #File Name    : AppLinuxEthMng.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppLinuxEthMng.h"
#include "Customers/NetworkService_Config.h"

#include "net_config.h"

#include "AppGlobalVariables.h"
#include "AppTaskManager.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef enum 
{
    LINUX_ETH_CONN_STEP_INIT , 
    LINUX_ETH_CONN_STEP_ETH_UP,
    LINUX_ETH_CONN_STEP_ETH_DOWN,
    LINUX_ETH_CONN_STEP_COMPLETED,
} LinuxEthModuleStep_t;

/********************************** VARIABLES *********************************/
static S32 gs_linuxEthDbusID;
static S32 gs_linuxEthTimerId;
OsTaskId gs_linuxEthTaskID;

static LinuxEthModuleStep_t gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_INIT;

/***************************** STATIC FUNCTIONS  ******************************/
static void linuxEthPeriodicInfoCb (void)
{
    DBUS_PACKET dbPacket;
    
    DEBUG_INFO("Publishing LINUX_ETH Data");

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_ETH;

    dbPacket.payload.data[0] = TRUE;

    appDBusPublish(gs_linuxEthDbusID, &dbPacket);
}

static void linuxEthConnManagerTask(void* argument)
{      
    (void)argument;
    zosDelayTask(1000);

    while(666)
    { 
        appTskMngImOK(gs_linuxEthTaskID);
        switch (gs_linuxEthInitStep)
        {
            case LINUX_ETH_CONN_STEP_INIT:
            {                
                gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_ETH_UP;
                DEBUG_INFO("LINUX_ETH driver initialized successfully");
                APP_LOG_REC(g_sysLoggerID, "LINUX_ETH driver initialized successfully");
            
                break;      
            }

            case LINUX_ETH_CONN_STEP_ETH_UP:
            {     
                gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_COMPLETED;
                DEBUG_INFO("LINUX_ETH connection established successfully!");
                APP_LOG_REC(g_sysLoggerID, "LINUX_ETH connection established successfully!");
                                
                break;
            }
            
            case LINUX_ETH_CONN_STEP_ETH_DOWN:
            {
                gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_COMPLETED;
                DEBUG_INFO("LINUX_ETH connection down !");
                APP_LOG_REC(g_sysLoggerID, "LINUX_ETH connection down !");                    
            
                break;
            }

            default:
                break;
        }
        
        appTskMngImOK(gs_linuxEthTaskID);
        zosDelayTask(1000);
    }
}

#endif
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS AppLinuxEthMngTcpIpStackInit(void)
{
    //not needed for linux
    return SUCCESS;
}

RETURN_STATUS AppLinuxEthMngStart(void)
{
    RETURN_STATUS retVal = SUCCESS;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    retVal = appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gs_linuxEthDbusID);
    if (SUCCESS != retVal)
    {
        DEBUG_ERROR("Failed to register LINUX_ETH manager to DBus!");
        APP_LOG_REC(g_sysLoggerID, "Failed to register LINUX_ETH manager to DBus!");
        return FAILURE;
    }

    //return value of timer is not critical, so we do not check it
    middEventTimerRegister(&gs_linuxEthTimerId, linuxEthPeriodicInfoCb, WAIT_1_MIN , TRUE);
    middEventTimerStart(gs_linuxEthTimerId);

    if (SUCCESS == retVal)
    {
        gs_linuxEthTaskID = appTskMngCreate("LINUX_ETH_TASK", linuxEthConnManagerTask, NULL, &tempParam);

        if (OS_INVALID_TASK_ID != gs_linuxEthTaskID)
        {
            DEBUG_ERROR("->[E] LINUX_ETH Task created id: %lu", gs_linuxEthTaskID);
            APP_LOG_REC(g_sysLoggerID, "LINUX_ETH: Task created successfully");
        }
        else
        {
            appDBusUnregister(gs_linuxEthDbusID);
            retVal = FAILURE;

            DEBUG_ERROR("->[E] LINUX_ETH Task could not be created");
            APP_LOG_REC(g_sysLoggerID, "LINUX_ETH: Task could not be created");
        }
    }

    return retVal;
}

RETURN_STATUS AppLinuxEthMngClose(void)
{    
    gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_ETH_DOWN;
    return SUCCESS;
}

RETURN_STATUS AppLinuxEthMngReconnect(void)
{    
    gs_linuxEthInitStep = LINUX_ETH_CONN_STEP_ETH_UP;
    return SUCCESS;
}

BOOL AppLinuxEthMngIsNetworkReady(void)
{
    return TRUE;
}

/******************************** End Of File *********************************/
