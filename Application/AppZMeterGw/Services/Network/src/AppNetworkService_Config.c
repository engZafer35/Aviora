/*
 * AppNetworkService_Config.c
 *
 * Network Service Configuration
 * Auto-generated from JSON device configuration
 *
 * This file contains:
 * - Global configuration structure (gAppNetworkServiceConfig)
 * - Interface initialization coordinator (priv_InitializeInterfaces)
 *
 * Individual interface implementations are in separate files:
 * - AppNetworkService_GSM.c (GSM/PPP interface)
 * - AppNetworkService_ETH.c (Ethernet interface)
 *
 * This file is #include'd by AppNetworkService_InterfaceLoader.c
 * DO NOT compile separately.
 */

/*==============================================================================
 * Global Network Service Configuration
 *============================================================================*/
#include "Project_Conf.h"
#include "net_config.h"

/* GSM Interface Implementation */
#if defined(GSM_INTERFACE_ENABLED) || 1  /* Replace with: if enabled in config */
#include "AppNetworkService_GSM.c"
#endif

/* Ethernet Interface Implementation */
#if defined(ETH_INTERFACE_ENABLED) || 1  /* Replace with: if enabled in config */
#include "AppNetworkService_ETH.c"
#endif
/**
 * Network Service Configuration
 * Auto-generated from JSON device configuration
 * Only enabled interfaces are included in this structure
 */

const AppNetworkServiceConfig_t gAppNetworkServiceConfig = {
    .gsmConfig = {
        .enabled = true,
        .devName = "quectelUC2000",
        .usePPP = true,
        .connInterface = "huart1",
        .srcPath = "/g/uc2000_atP"
        .initFunc = AppNetworkService_InitGsmInterface,
        .connectFunc = AppNetworkService_ConnectGsm,
        .disconnectFunc = AppNetworkService_DisconnectGsm,
        .startConnecting = FALSE,
    },
    .ethConfig = {
        .enabled = true,
        .devName = "ENC28J60",
        .connInterface = "hspi2",
        .srcPath = "/e/ENC28J60"
        .initFunc = AppNetworkService_InitEthInterface,
        .connectFunc = AppNetworkService_ConnectEth,
        .disconnectFunc = AppNetworkService_DisconnectEth,
        .startConnecting = FALSE,
    },
    .interfaceCount = 2
};

/**
 * priv_InitTCPIPStack - if any tcp/ip stack is used in the project, it should be initialized
 * but if no stack is used, this function can be left empty and return SUCCESS.
 */
static RETURN_STATUS priv_InitTCPIPStack(void)
{
    RETURN_STATUS retVal = SUCCESS;

    //TCP/IP stack initialization
    if(NO_ERROR != netInit())
    {
        retVal = FAILURE;
        DEBUG_ERROR("Failed to initialize TCP/IP stack!");
    }

    return retVal;
}

/*==============================================================================
 * Interface Initialization and Deinitialization Coordinators
 *============================================================================*/

/**
 * priv_InitializeInterfaces - Initialize Active Network Interfaces
 *
 * This function is called by AppNetworkService_Init() to initialize
 * all enabled interfaces. It gracefully handles per-interface failures.
 */
static int32_t priv_StartInterfaces(void)
{
    RETURN_STATUS retVal = SUCCESS;

    /* === GSM Interface Initialization === */
    //TODO: initialize all gsm interface listed in config
    if (SUCCESS != AppNetworkService_InitGsmInterface(&gAppNetworkServiceConfig.gsmConfig)) 
    {
        retVal = FAILURE;
        DEBUG_ERROR("->[E] InitGsmInterface failed");
        /* Continue anyway - Another interface may still be available */
    }
    
    /* === Ethernet Interface Initialization === */
    //TODO: initialize all eth interface listed in config
    if (SUCCESS != AppNetworkService_InitEthInterface(&gAppNetworkServiceConfig.ethConfig)) 
    {
        retVal = FAILURE;
        DEBUG_ERROR("->[E] InitEthInterface failed");
        /* Continue anyway - Another interface may still be available */
    }

    return retVal;
}

/**
 * priv_DeinitializeInterfaces - Disconnect and Deinitialize Active Interfaces
 *
 * This function is called by AppNetworkService_Deinit() to disconnect
 * all enabled interfaces. Only connected interfaces are disconnected.
 */
static RETURN_STATUS priv_DeinitializeInterfaces(void)
{
    RETURN_STATUS retVAl = SUCCESS;

    /* === GSM Interface Disconnection === */
    if (gNetworkService_state.gsmState.connected) 
    {
        if (SUCCESS != AppNetworkService_DisconnectGsm()) 
        {
            retVAl = FAILURE;
            DEBUG_ERROR("->[E] AppNetworkService: DisconnectGsm failed");
            /* Continue anyway - need to disconnect other interfaces */
        }
    }

    /* === Ethernet Interface Disconnection === */
    if (gNetworkService_state.ethState.connected) 
    {
        if (SUCCESS != AppNetworkService_DisconnectEth()) 
        {
            retVAl = FAILURE;
            DEBUG_ERROR("->[E] AppNetworkService: DisconnectEth failed");
            /* Continue anyway - other cleanup must proceed */
        }
    }

    return retVAl;
}

/*==============================================================================
 * Interface Status Query Helper
 *============================================================================*/

/**
 * AppNetworkService_GetActiveInterfaceCount - Get Active Interface Count
 * Returns the number of currently initialized network interfaces
 */
int32_t AppNetworkService_GetActiveInterfaceCount(void)
{
    int32_t count = 0;

    if (gNetworkService_state.gsmState.initialized) {
        count++;
    }

    if (gNetworkService_state.ethState.initialized) {
        count++;
    }

    return count;
}

/*** End Of File ***/


