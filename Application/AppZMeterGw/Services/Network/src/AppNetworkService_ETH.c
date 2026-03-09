/*
 * AppNetworkService_ETH.c
 *
 * Ethernet Interface Implementation
 * Handles Ethernet network interface (e.g., ENC28J60)
 *
 * This file is #include'd by AppNetworkService_InterfaceLoader.c
 * when Ethernet interface is enabled in configuration.
 * DO NOT compile this file separately.
 */

/*==============================================================================
 * Ethernet Interface Initialization
 *============================================================================*/

int32_t AppNetworkService_InitEthInterface(const AppNetworkEthConfig_t* config)
{
    int32_t result = 0;

    if (config == NULL || !config->enabled) {
        return -1;
    }

    /* TODO: Initialize Ethernet interface
     * Config parameters available:
     * - Device name: config->devName (e.g., "ENC28J60")
     * - SPI interface: config->connInterface (e.g., "hspi2")
     * - Resource path: config->srcPath
     *
     * Example steps:
     * - Configure SPI communication based on connInterface
     * - Initialize Ethernet driver
     * - Set up network parameters (MAC address, etc.)
     */

    gNetworkService_state.ethState.initialized = true;

    return 0;
}

/*==============================================================================
 * Ethernet Interface Connection
 *============================================================================*/

int32_t AppNetworkService_ConnectEth(void)
{
    int32_t result = 0;

    if (!gNetworkService_state.ethState.initialized) {
        return -1;
    }

    /* TODO: Connect Ethernet interface
     * - Configure network parameters (IP, gateway, etc.)
     * - Enable Ethernet driver
     * - Perform link detection
     */

    gNetworkService_state.ethState.connected = true;
    priv_PublishNetworkEvent(0x11, 1, 0); /* ETH_CONNECTED */

    return 0;
}

/*==============================================================================
 * Ethernet Interface Disconnection
 *============================================================================*/

int32_t AppNetworkService_DisconnectEth(void)
{
    int32_t result = 0;

    if (!gNetworkService_state.ethState.initialized) {
        return -1;
    }

    /* TODO: Disconnect Ethernet interface
     * - Disable Ethernet driver
     * - Release network resources
     */

    gNetworkService_state.ethState.connected = false;
    priv_PublishNetworkEvent(0x12, 1, 0); /* ETH_DISCONNECTED */

    return 0;
}

/*==============================================================================
 * Ethernet Interface Status
 *============================================================================*/

int32_t AppNetworkService_IsEthConnected(bool* isConnected)
{
    if (isConnected == NULL) {
        return -1;
    }

    *isConnected = gNetworkService_state.ethState.connected;
    return 0;
}

/*** End Of File ***/
