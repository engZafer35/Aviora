/*
 * AppNetworkService.h
 *
 * Network Service - Handles network interface initialization and management
 * Supports GSM and Ethernet interfaces based on device configuration
 *
 * Created on: Mar 04, 2026
 */

#ifndef APPLICATION_APPZMETERGW_SERVICES_NETWORK_INC_APPNETWORKSERVICE_H_
#define APPLICATION_APPZMETERGW_SERVICES_NETWORK_INC_APPNETWORKSERVICE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Configuration Structures - Generated from JSON by external Python parser
 *============================================================================*/

/**
 * GSM Interface Configuration
 * Generated from JSON GSM configuration block
 */
typedef struct {
    bool enabled;                   /**< GSM interface enabled flag */
    const char* devName;            /**< Device name (e.g., "quectelUC2000") */
    bool usePPP;                    /**< Use PPP protocol for connection */
    const char* connInterface;      /**< Connection interface (e.g., "huart1") */
    const char* srcPath;            /**< Resource path (e.g., "/g/uc2000_atP") */
} AppNetworkGsmConfig_t;

/**
 * Ethernet Interface Configuration
 * Generated from JSON Ethernet configuration block
 */
typedef struct {
    bool enabled;                   /**< Ethernet interface enabled flag */
    const char* devName;            /**< Device name (e.g., "ENC28J60") */
    const char* connInterface;      /**< Connection interface (e.g., "hspi2") */
    const char* srcPath;            /**< Resource path (e.g., "/e/ENC28J60") */
} AppNetworkEthConfig_t;

/**
 * Complete Network Service Configuration
 * Contains configuration for all available network interfaces
 */
typedef struct {
    AppNetworkGsmConfig_t gsmConfig;        /**< GSM interface configuration */
    AppNetworkEthConfig_t ethConfig;        /**< Ethernet interface configuration */
    uint8_t interfaceCount;                 /**< Total number of enabled interfaces */
} AppNetworkServiceConfig_t;

/*==============================================================================
 * Service State Structures
 *============================================================================*/

/**
 * GSM Interface State
 */
typedef struct {
    bool initialized;               /**< Initialization status */
    bool connected;                 /**< Connection status */
    const void* gsmManagerHandle;   /**< Handle to AppGsmManager instance */
} AppNetworkGsmState_t;

/**
 * Ethernet Interface State
 */
typedef struct {
    bool initialized;               /**< Initialization status */
    bool connected;                 /**< Connection status */
    const void* ethManagerHandle;   /**< Handle to Ethernet manager instance */
} AppNetworkEthState_t;

/**
 * Network Service State
 */
typedef struct {
    bool serviceInitialized;        /**< Service initialization flag */
    bool taskRunning;               /**< Task running status */
    AppNetworkGsmState_t gsmState;  /**< GSM interface state */
    AppNetworkEthState_t ethState;  /**< Ethernet interface state */
} AppNetworkServiceState_t;

/*==============================================================================
 * Service Lifecycle Functions
 *============================================================================*/

/**
 * Initialize the Network Service
 *
 * Sets up the network service with the provided configuration.
 * Creates and starts the network management task.
 *
 * @param config    Pointer to network service configuration
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_Init(void);

/**
 * Deinitialize the Network Service
 *
 * Shuts down all network interfaces and stops the service task.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_Deinit(void);

/**
 * Start the Network Service Task
 *
 * Starts the network management task after initialization.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_StartTask(void);

/**
 * Stop the Network Service Task
 *
 * Stops the network management task.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_StopTask(void);

/*==============================================================================
 * Interface Management Functions
 *============================================================================*/

/**
 * Initialize GSM Interface
 *
 * Sets up the GSM interface based on configuration.
 * Creates AppGsmManager instance and configures connection parameters.
 *
 * @param config    Pointer to GSM configuration
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_InitGsmInterface(const AppNetworkGsmConfig_t* config);

/**
 * Initialize Ethernet Interface
 *
 * Sets up the Ethernet interface based on configuration.
 * Configures the specified SPI/communication interface and network parameters.
 *
 * @param config    Pointer to Ethernet configuration
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_InitEthInterface(const AppNetworkEthConfig_t* config);

/**
 * Connect GSM Interface
 *
 * Establishes connection for the GSM interface.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_ConnectGsm(void);

/**
 * Connect Ethernet Interface
 *
 * Establishes connection for the Ethernet interface.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_ConnectEth(void);

/**
 * Disconnect GSM Interface
 *
 * Disconnects the GSM interface.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_DisconnectGsm(void);

/**
 * Disconnect Ethernet Interface
 *
 * Disconnects the Ethernet interface.
 *
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_DisconnectEth(void);

/*==============================================================================
 * Status and Query Functions
 *============================================================================*/

/**
 * Get Current Service State
 *
 * Retrieves the current state of all network interfaces and service.
 *
 * @param state     Pointer to state structure to be filled
 * @return          0 on success, negative value on error
 */
int32_t AppNetworkService_GetState(AppNetworkServiceState_t* state);

/**
 * Get GSM Interface Status
 *
 * Retrieves the current status of GSM interface.
 *
 * @param isConnected   Pointer to boolean to receive connection status
 * @return              0 on success, negative value on error
 */
int32_t AppNetworkService_IsGsmConnected(bool* isConnected);

/**
 * Get Ethernet Interface Status
 *
 * Retrieves the current status of Ethernet interface.
 *
 * @param isConnected   Pointer to boolean to receive connection status
 * @return              0 on success, negative value on error
 */
int32_t AppNetworkService_IsEthConnected(bool* isConnected);

/**
 * Get Active Interface Count
 *
 * Returns the number of currently enabled network interfaces.
 *
 * @return              Number of active interfaces, or negative on error
 */
int32_t AppNetworkService_GetActiveInterfaceCount(void);

/*==============================================================================
 * Event Publishing via DataBus
 *============================================================================*/

/**
 * Publish Network Status via DataBus
 *
 * Network service publishes connection status and interface information
 * to EN_DBUS_TOPIC_NETWORK topic. Other services subscribe to this topic
 * to receive network state updates (connection established, connection lost,
 * signal level, etc.)
 *
 * DataBus provides a decoupled pub/sub mechanism for inter-service communication.
 */

/*==============================================================================
 * Service Task Function
 *============================================================================*/

/**
 * Network Service Task
 *
 * Main task function for the network service.
 * Manages interface health monitoring, reconnection attempts, and state changes.
 * Called by the task scheduler with a period defined in configuration.
 *
 * @param argument  Optional argument passed by task scheduler
 */
void AppNetworkService_Task(void* argument);

/*==============================================================================
 * Configuration and Initialization (Auto-generated by Python config parser)
 *============================================================================*/

/**
 * External configuration generated by Python configuration parser.
 * This is defined in AppNetworkService_Config.c which is auto-generated
 * based on the device configuration JSON file.
 */
extern const AppNetworkServiceConfig_t gAppNetworkServiceConfig;

/**
 * Interface initialization function - Auto-generated by Python config parser.
 * This function is defined in AppNetworkService_Config.c and contains
 * only the initialization code for enabled interfaces.
 * Disabled interfaces are not included in the generated code.
 *
 * @return  0 on success, negative value on error
 */
extern int32_t priv_InitializeInterfaces(void);

/**
 * Interface deinitialization function - Auto-generated by Python config parser.
 * This function is defined in AppNetworkService_Config.c and disconnects
 * all enabled interfaces. Disabled interfaces are not disconnected.
 *
 * @return  0 on success, negative value on error
 */
extern int32_t priv_DeinitializeInterfaces(void);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_APPZMETERGW_SERVICES_NETWORK_INC_APPNETWORKSERVICE_H_ */
