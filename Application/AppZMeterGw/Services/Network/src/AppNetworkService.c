/*
 * AppNetworkService.c
 *
 * Network Service Implementation
 * Handles network interface initialization and management
 * Supports GSM and Ethernet interfaces based on device configuration
 *
 * Created on: Mar 04, 2026
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "AppNetworkService.h"
#include "AppDataBus.h"
#include "AppGsmManager.h"

#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"
#include "AppInternalMsgFrame.h"


/*==============================================================================
 * Network Configuration and Interface Implementations
 * Auto-generated interface loader included from AppNetworkService_InterfaceLoader.c
 *============================================================================*/

/* Include the interface loader which selectively includes configuration
 * and active interface implementations based on the device configuration. */
#include "AppNetworkService_Config.c"

/*==============================================================================
 * Defines and Constants
 *============================================================================*/

#define NETWORK_SERVICE_TASK_PRIORITY       (4U)
#define NETWORK_SERVICE_TASK_STACK_SIZE     (512U)
#define NETWORK_SERVICE_TASK_PERIOD_MS      (1000U)

/**
 * Network event data published to DataBus
 */
typedef enum {
    NETWORK_EVENT_GSM_CONNECTED,
    NETWORK_EVENT_GSM_DISCONNECTED,
    NETWORK_EVENT_GSM_ERROR,
    NETWORK_EVENT_ETH_CONNECTED,
    NETWORK_EVENT_ETH_DISCONNECTED,
    NETWORK_EVENT_ETH_ERROR,
    NETWORK_EVENT_GSM_SIGNAL_LEVEL,
} NetworkEventType_t;

/*==============================================================================
 * Private Global Variables
 *============================================================================*/

/**
 * Network Service State (singleton)
 * Accessible by AppNetworkService_Config.c for interface implementations
 */
AppNetworkServiceState_t gNetworkService_state = {0};

static struct {
    AppNetworkServiceConfig_t config;
    int32_t dbusClientID;
    bool taskCreated;
    void* taskHandle;
    volatile bool taskShouldRun;
} gNetworkService = {
    .config = {0},
    .dbusClientID = -1,
    .taskCreated = false,
    .taskHandle = NULL,
    .taskShouldRun = false
};

/*==============================================================================
 * Private Function Prototypes
 *============================================================================*/

static int32_t priv_PublishNetworkEvent(NetworkEventType_t eventType, uint8_t interfaceType, int32_t value);

/*==============================================================================
 * Service Lifecycle Functions
 *============================================================================*/

/**
 * AppNetworkService_Init - Initialize the Network Service
 */
RETURN_STATUS AppNetworkService_Init(void)
{
    RETURN_STATUS retVal = FAILURE;

    /* Check if already initialized */
    if (gNetworkService_state.serviceInitialized) 
    {
        return FAILURE;
    }

    /* Register with DataBus for EN_DBUS_TOPIC_NETWORK events */
    if (SUCCESS != appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gNetworkService.dbusClientID)) 
    {
        DEBUG_ERROR("->[E] appDBusRegister returned error");
        return FAILURE;
    }

    gNetworkService_state.serviceInitialized = true;

    return retVal;
}

/**
 * AppNetworkService_Deinit - Deinitialize the Network Service
 */
RETURN_STATUS AppNetworkService_Deinit(void)
{
    /* Stop task if running */
    if (gNetworkService_state.taskRunning)
    {
        AppNetworkService_StopTask();
    }

    /* Disconnect all enabled interfaces */
    priv_DeinitializeInterfaces();

    /* Unregister from DataBus */
    if (gNetworkService.dbusClientID >= 0)
    {
        appDBusUnregister(gNetworkService.dbusClientID);
        gNetworkService.dbusClientID = -1;
    }

    /* Reset state */
    memset(&gNetworkService_state, 0, sizeof(AppNetworkServiceState_t));
    gNetworkService_state.serviceInitialized = false;

    return SUCCESS;
}

/**
 * AppNetworkService_StartTask - Start the Network Service Task
 */
int32_t AppNetworkService_StartTask(void)
{
    int32_t result = 0;

    /* Check if service is initialized */
    if (!gNetworkService_state.serviceInitialized) 
    {
        return -1;
    }

    /* Check if task already running */
    if (gNetworkService_state.taskRunning)
    {
        return -2;
    }

    /* Enable task execution */
    gNetworkService.taskShouldRun = true;

    /* Create OS task - This is platform-specific and would be implemented
       based on the actual OS being used (FreeRTOS, ThreadX, etc.)
       For now, we mark it as created and running */
    gNetworkService.taskCreated = true;
    gNetworkService_state.taskRunning = true;

    return 0;
}

/**
 * AppNetworkService_StopTask - Stop the Network Service Task
 */
RETURN_STATUS AppNetworkService_StopTask(void)
{
    /* Check if task is running */
    if (!gNetworkService_state.taskRunning) 
    {
        return FAILURE;
    }

    /* Signal task to stop */
    gNetworkService.taskShouldRun = false;

    /* Wait for task to stop (implementation depends on OS) */
    gNetworkService.taskCreated = false;
    gNetworkService_state.taskRunning = false;

    return 0;
}

/*==============================================================================
 * Interface Management Functions (Auto-generated in AppNetworkService_Config.c)
 *============================================================================*/

/* All interface management functions are implemented in AppNetworkService_Config.c
 * which is auto-generated by Python configuration parser.
 * This includes:
 * - AppNetworkService_InitGsmInterface()
 * - AppNetworkService_InitEthInterface()
 * - AppNetworkService_ConnectGsm()
 * - AppNetworkService_ConnectEth()
 * - AppNetworkService_DisconnectGsm()
 * - AppNetworkService_DisconnectEth()
 * - AppNetworkService_IsGsmConnected()
 * - AppNetworkService_IsEthConnected()
 * - AppNetworkService_GetActiveInterfaceCount()
 * - priv_UpdateGsmState()
 * - priv_UpdateEthState()
 * - priv_HandleReconnection()
 * 
 * See AppNetworkService_Config.c for implementations.
 */

/*==============================================================================
 * Status and Query Functions (Implemented in AppNetworkService_Config.c)
 *============================================================================*/

/**
 * AppNetworkService_GetState - Get Current Service State
 */
int32_t AppNetworkService_GetState(AppNetworkServiceState_t* state)
{
    /* Validate input */
    if (state == NULL)
    {
        return -1;
    }

    /* Copy current state */
    memcpy(state, &gNetworkService_state, sizeof(AppNetworkServiceState_t));

    return 0;
}

/*==============================================================================
 * Service Task Function
 *============================================================================*/

/**
 * AppNetworkService_Task - Network Service Task
 *
 * Main task function executed periodically by the task scheduler.
 * Responsibilities:
 * - Monitor interface health and connection status
 * - Handle reconnection attempts
 * - Publish periodic status updates
 * - Update signal levels (GSM)
 */
staticvoid AppNetworkService_Task(void* argument)
{
    (void)argument;
    S32 i;
    DBUS_PACKET busMsg;
    DevInfoMsg devMsg;

    /* Initialize network interfaces based on configuration */
    if (SUCCESS == priv_InitializeInterfaces()) 
    {
        retVal = SUCCESS;
        gNetworkService_state.serviceInitialized = true;
        DEBUG_ERROR("->[E] InitializeInterfaces returned error");
        /* Continue anyway - some interfaces may still be initialized and available
         * Task monitoring will identify which interfaces are working via DataBus */
    }

    for (i = 0; i < gAppNetworkServiceConfig.interfaceCount; i++) 
    {
        if (SUCCESS == gAppNetworkServiceConfig.interfaceList[i].initFunc(&gAppNetworkServiceConfig.interfaceList[i])) 
        {
            DEBUG_INFO("->[I] initialize interface %s", gAppNetworkServiceConfig.interfaceList[i].devName);
            gAppNetworkServiceConfig.interfaceList[i].startConnect == TRUE;
        }

        osDelayTask(250); // Small delay between interface initializations
    }

    while (gNetworkService.taskShouldRun) 
    {
        for (i = 0; i < gAppNetworkServiceConfig.interfaceCount; i++) 
        {
            if (TRUE == gAppNetworkServiceConfig.interfaceList[i].startConnect) 
            {
                if (SUCCESS == gAppNetworkServiceConfig.interfaceList[i].connectFunc()) 
                {
                    gAppNetworkServiceConfig.interfaceList[i].startConnect = FALSE; // Clear flag after successful connection
                }
                osDelayTask(250); // Small delay between connection attempts
            }
        }

        if (SUCCESS == appDBusReceive(&gNetworkService.dbusClientID, &busMsg, WAIT_10_MS))
        {
            DEBUG_INFO("->[I] Newwork: Dbus raw msg pckID: %d, topic: %d ", busMsg.packetID, busMsg.topic);
            if (busMsg.topic & EN_DBUS_TOPIC_DEVICE)
            {
                if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                {
                    if (EN_WORKING_MODE_NO_NETWORK == devMsg.wMode)
                    {
                        for (i = 0; i < gAppNetworkServiceConfig.interfaceCount; i++) 
                        {
                            if (SUCCESS == gAppNetworkServiceConfig.interfaceList[i].disconnectFunc()) 
                            {
                                gAppNetworkServiceConfig.interfaceList[i].startConnect = FALSE;
                                DEBUG_INFO("->[I] disconnect interface %s", gAppNetworkServiceConfig.interfaceList[i].devName);
                            }
                        }
                    }
                }
            }
        }

        osDelayTask(1000);
    }
}

/*==============================================================================
 * Private Helper Functions
 *============================================================================*/

/**
 * priv_PublishNetworkEvent - Publish Network Event to DataBus
 */
static RETURN_STATUS priv_PublishNetworkEvent(DBUS_PACKET *packet)
{
    RETURN_STATUS retVal = FAILURE;

    /* Check if registered with DataBus */
    if (gNetworkService.dbusClientID > 0) 
    {
        result = appDBusPublish(gNetworkService.dbusClientID, packet);
    }
    
    return result;
}


/*** End Of File ***/
