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
    NETWORK_EVENT_GSM_CONNECTED = 0x01,
    NETWORK_EVENT_GSM_DISCONNECTED = 0x02,
    NETWORK_EVENT_GSM_ERROR = 0x03,
    NETWORK_EVENT_ETH_CONNECTED = 0x11,
    NETWORK_EVENT_ETH_DISCONNECTED = 0x12,
    NETWORK_EVENT_ETH_ERROR = 0x13,
    NETWORK_EVENT_GSM_SIGNAL_LEVEL = 0x21,
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
int32_t AppNetworkService_Init(void)
{
    int32_t result = 0;

    /* Check if already initialized */
    if (gNetworkService_state.serviceInitialized) {
        return -2;
    }

    /* Register with DataBus for EN_DBUS_TOPIC_NETWORK events */
    result = appDBusRegister(EN_DBUS_TOPIC_NETWORK, &gNetworkService.dbusClientID);
    if (result != 0) {
        return -3;
    }

    /* Initialize network interfaces based on configuration */
    result = priv_InitializeInterfaces();
    if (result != 0) 
    {
        DEBUG_ERROR("->[E] AppNetworkService: priv_InitializeInterfaces returned error");
        /* Continue anyway - some interfaces may still be initialized and available
         * Task monitoring will identify which interfaces are working via DataBus */
    }

    /* Mark service as initialized */
    gNetworkService_state.serviceInitialized = true;

    return 0;
}

/**
 * AppNetworkService_Deinit - Deinitialize the Network Service
 */
int32_t AppNetworkService_Deinit(void)
{
    /* Stop task if running */
    if (gNetworkService_state.taskRunning) {
        AppNetworkService_StopTask();
    }

    /* Disconnect all enabled interfaces */
    priv_DeinitializeInterfaces();

    /* Unregister from DataBus */
    if (gNetworkService.dbusClientID >= 0) {
        appDBusUnregister(gNetworkService.dbusClientID);
        gNetworkService.dbusClientID = -1;
    }

    /* Reset state */
    memset(&gNetworkService_state, 0, sizeof(AppNetworkServiceState_t));
    gNetworkService_state.serviceInitialized = false;

    return 0;
}

/**
 * AppNetworkService_StartTask - Start the Network Service Task
 */
int32_t AppNetworkService_StartTask(void)
{
    int32_t result = 0;

    /* Check if service is initialized */
    if (!gNetworkService_state.serviceInitialized) {
        return -1;
    }

    /* Check if task already running */
    if (gNetworkService_state.taskRunning) {
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
int32_t AppNetworkService_StopTask(void)
{
    /* Check if task is running */
    if (!gNetworkService_state.taskRunning) {
        return -1;
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
    if (state == NULL) {
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
void AppNetworkService_Task(void* argument)
{
    (void)argument;

    /* Task loop */
    while (gNetworkService.taskShouldRun) {
        /* Update interface states */
        priv_UpdateGsmState();
        priv_UpdateEthState();

        /* Handle reconnection logic if needed */
        priv_HandleReconnection();

        /* Sleep for configured period (NETWORK_SERVICE_TASK_PERIOD_MS) */
        /* This is platform-specific delay - implement based on actual OS */
        /* os_delay_ms(NETWORK_SERVICE_TASK_PERIOD_MS); */
    }
}

/*==============================================================================
 * Private Helper Functions
 *============================================================================*/

/**
 * priv_PublishNetworkEvent - Publish Network Event to DataBus
 */
static int32_t priv_PublishNetworkEvent(NetworkEventType_t eventType, uint8_t interfaceType, int32_t value)
{
    DBUS_PACKET packet;
    int32_t result = 0;

    /* Check if registered with DataBus */
    if (gNetworkService.dbusClientID < 0) {
        return -1;
    }

    /* Prepare packet */
    memset(&packet, 0, sizeof(DBUS_PACKET));

    packet.topic = EN_DBUS_TOPIC_NETWORK;
    packet.pri = EN_PRIORITY_MED;
    packet.retainFlag = 1;  /* Retain this packet for new subscribers */
    packet.payload.dataLeng = 3;

    /* Encode event data in payload */
    packet.payload.data[0] = eventType;
    packet.payload.data[1] = interfaceType;
    packet.payload.data[2] = (uint8_t)(value & 0xFF);

    /* Publish to DataBus */
    result = appDBusPublish(gNetworkService.dbusClientID, &packet);

    return result;
}

/*==============================================================================
 * Network Configuration and Interface Implementations
 * Auto-generated interface loader included from AppNetworkService_InterfaceLoader.c
 *============================================================================*/

/* Include the interface loader which selectively includes configuration
 * and active interface implementations based on the device configuration. */
#include "AppNetworkService_InterfaceLoader.c"

/*** End Of File ***/
