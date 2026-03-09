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
    },
    .ethConfig = {
        .enabled = true,
        .devName = "ENC28J60",
        .connInterface = "hspi2",
        .srcPath = "/e/ENC28J60"
    },
    .interfaceCount = 2
};

/*==============================================================================
 * Interface Initialization and Deinitialization Coordinators
 *============================================================================*/

/**
 * priv_InitializeInterfaces - Initialize Active Network Interfaces
 *
 * This function is called by AppNetworkService_Init() to initialize
 * all enabled interfaces. It gracefully handles per-interface failures.
 */
int32_t priv_InitializeInterfaces(void)
{
    int32_t result = 0;

    /* === GSM Interface Initialization === */
    if (gAppNetworkServiceConfig.gsmConfig.enabled) {
        result = AppNetworkService_InitGsmInterface(&gAppNetworkServiceConfig.gsmConfig);
        if (result != 0) {
            DEBUG_ERROR("->[E] AppNetworkService: InitGsmInterface failed");
            /* Continue anyway - Ethernet may still be available */
        }
    }

    /* === Ethernet Interface Initialization === */
    if (gAppNetworkServiceConfig.ethConfig.enabled) {
        result = AppNetworkService_InitEthInterface(&gAppNetworkServiceConfig.ethConfig);
        if (result != 0) {
            DEBUG_ERROR("->[E] AppNetworkService: InitEthInterface failed");
            /* Continue anyway - GSM may still be available */
        }
    }

    return 0;
}

/**
 * priv_DeinitializeInterfaces - Disconnect and Deinitialize Active Interfaces
 *
 * This function is called by AppNetworkService_Deinit() to disconnect
 * all enabled interfaces. Only connected interfaces are disconnected.
 */
int32_t priv_DeinitializeInterfaces(void)
{
    int32_t result = 0;

    /* === GSM Interface Disconnection === */
    if (gAppNetworkServiceConfig.gsmConfig.enabled && gNetworkService_state.gsmState.connected) {
        result = AppNetworkService_DisconnectGsm();
        if (result != 0) {
            DEBUG_ERROR("->[E] AppNetworkService: DisconnectGsm failed");
            /* Continue anyway - need to disconnect other interfaces */
        }
    }

    /* === Ethernet Interface Disconnection === */
    if (gAppNetworkServiceConfig.ethConfig.enabled && gNetworkService_state.ethState.connected) {
        result = AppNetworkService_DisconnectEth();
        if (result != 0) {
            DEBUG_ERROR("->[E] AppNetworkService: DisconnectEth failed");
            /* Continue anyway - other cleanup must proceed */
        }
    }

    return 0;
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


