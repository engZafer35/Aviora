/*
 * AppNetworkService_Config.c
 *
 * Network Service Configuration
 * Auto-generated from JSON device configuration
 *
 * This file contains:
 * - Global configuration structure (gAppNetworkServiceConfig)
 * - Interface initialization coordinator (priv_StartInterfaces)
 *
 * Individual interface implementations are in separate files:
 * - AppNetworkService_GSM.c (GSM/PPP interface)
 * - AppNetworkService_ETH.c (Ethernet interface)
 *
 * This file is #include'd by AppNetworkService.c
 * DO NOT compile separately.
 */

/*==============================================================================
 * Global Network Service Configuration
 *============================================================================*/
#include "Project_Conf.h"
#include "net_config.h"

/* GSM Interface Implementation */
#if defined(GSM_INTERFACE_ENABLED) || 1  /* Replace with: if enabled in config */
#include "GsmConn_Comm.h"
#endif

/* Ethernet Interface Implementation */
#if defined(ETH_INTERFACE_ENABLED) || 1  /* Replace with: if enabled in config */
#include "EthConn_ENC28j60.h"
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

    },
    .ethConfig = {
        .enabled = true,
        .devName = "ENC28J60",
        .connInterface = "hspi2",
        .srcPath = "/e/ENC28J60"
        //todo: add ethernet stack name, pc, tcp, lwIP, etc. if needed

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

/**
 * @brief   Start Network Interfaces
 * @return  if everything is OK, return EN_SUCCESS
 */
static RETURN_STATUS priv_StartInterfaces(void)
{
    RETURN_STATUS retVal = SUCCESS;

    /* === GSM Interface Initialization === */
    //TODO: initialize all gsm interface listed in config
    if (SUCCESS != GsmConnInit()) 
    {
        retVal = FAILURE;
        DEBUG_ERROR("->[E] InitGsmInterface failed");
        /* Continue anyway - Another interface may still be available */
    }
    
    /* === Ethernet Interface Initialization === */
    //TODO: initialize all eth interface listed in config
    if (SUCCESS != EthConn_Enc28J60_Init()) 
    {
        retVal = FAILURE;
        DEBUG_ERROR("->[E] InitEthInterface failed");
        /* Continue anyway - Another interface may still be available */
    }

    return retVal;
}

/**
 * @brief   Get Active Interface Count
 * @return  Number of currently initialized network interfaces
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


