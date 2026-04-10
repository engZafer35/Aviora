/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : AMar 04, 2026 - 1:43:59 PM
* #File Name    : AppInternalMsgFrame.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppNetworkService.h"
#include "AppDataBus.h"

#include "AppTaskManager.h"
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"
#include "AppInternalMsgFrame.h"

#include "AppNetworkService_Config.c"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
 typedef enum 
{    
    NM_STATE_INIT_TCPIP_STACK,
    NM_STATE_INIT_IFACES,
    NM_STATE_DONE,
} NetworkServiceState_t;

typedef enum {
    NET_IF_DOWN,
    NET_IF_CONNECTING,
    NET_IF_UP,
    NET_IF_ERROR
} NetIfState;

/********************************** VARIABLES *********************************/


/***************************** STATIC FUNCTIONS  ******************************/
static void networkServiceTask(void* argument)
{    
    NetworkServiceState_t nmState = NM_STATE_INIT_TCPIP_STACK;
    
    (void)argument;

    zosDelayTask(2000);

    while (1) 
    {
        switch (nmState)
        {
            case NM_STATE_INIT_TCPIP_STACK:
            {
                if (SUCCESS == priv_InitTCPIPStack())
                {
                    nmState = NM_STATE_INIT_IFACES;
                }
                break;
            }

            case NM_STATE_INIT_IFACES:
            {                
                if (SUCCESS == priv_StartInterfaces()) 
                {
                    nmState = NM_STATE_DONE;
                }
                break;
            }

            case NM_STATE_DONE:
            {
                zosDeleteTask(OS_SELF_TASK_ID);
                break;
            }
        }

        zosDelayTask(1000);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS AppNetworkService_Start(void)
{
    RETURN_STATUS retVal = FAILURE;
    ZOsTaskParameters tempParam;
    OsTaskId networkTaskID;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    networkTaskID = appTskMngCreate("NETWORK_TASK", networkServiceTask, NULL, &tempParam);

    if (OS_INVALID_TASK_ID != networkTaskID)
    {
        retVal = SUCCESS;
        DEBUG_ERROR("->[E] Network Task created id: %d", networkTaskID);
        APP_LOG_REC(g_sysLoggerID, "Network: Task created successfully");
    }
    else
    {
        DEBUG_ERROR("->[E] Network Task could not be created");
        APP_LOG_REC(g_sysLoggerID, "Network: Task could not be created");
    }

    return retVal;
}

/*** End Of File ***/
