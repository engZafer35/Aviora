/*
 * AppNetworkService.h
 *
 * Network Service - Handles network interface initialization and management
 * Supports GSM and Ethernet interfaces based on device configuration
 *
 * Created on: Mar 04, 2026
 */

#ifndef __APP_NETWORK_SERVICE_H__
#define __APP_NETWORK_SERVICE_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
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





#ifdef __cplusplus
}
#endif

#endif /* __APP_NETWORK_SERVICE_H__ */
