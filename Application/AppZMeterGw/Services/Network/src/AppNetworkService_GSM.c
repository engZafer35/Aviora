/*
 * AppNetworkService_GSM.c
 *
 * GSM Interface Implementation
 * Handles GSM/PPP network interface via AppGsmManager
 *
 * This file is #include'd by AppNetworkService_InterfaceLoader.c
 * when GSM interface is enabled in configuration.
 * DO NOT compile this file separately.
 */

/*==============================================================================
 * GSM Interface Initialization
 *============================================================================*/

int32_t AppNetworkService_InitGsmInterface(const AppNetworkGsmConfig_t* config)
{
    int32_t result = 0;

    if (config == NULL || !config->enabled) {
        return -1;
    }

    /* Initialize GSM Manager (TCP/IP stack, driver, etc.) - one-time only */
    result = appGsmMngInit();
    if (result != SUCCESS)
    {
        DEBUG_ERROR("->[E] appGsmMngInit failed");
        return -2;
    }

    /* Mark GSM interface as initialized */
    gNetworkService_state.gsmState.initialized = true;

    return 0;
}

/*==============================================================================
 * GSM Interface Connection
 *============================================================================*/

int32_t AppNetworkService_ConnectGsm(void)
{
    int32_t result = 0;
    DBUS_PACKET dbPacket;

    

    if (!gNetworkService_state.gsmState.initialized) {
        return -1;
    }

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = FALSE;
    dbPacket.topic      = EN_DBUS_TOPIC_NETWORK;

    /* Open PPP connection via AppGsmManager */
    result = appGsmMngOpenPPP();
    if (result != SUCCESS)
    {
        DEBUG_ERROR("->[E] appGsmMngOpenPPP failed");

        
        priv_PublishNetworkEvent(0x03, 0, result); /* GSM_ERROR */
        return -2;
    }

    gNetworkService_state.gsmState.connected = true;
    priv_PublishNetworkEvent(0x01, 0, 0); /* GSM_CONNECTED */

    return 0;
}

/*==============================================================================
 * GSM Interface Disconnection
 *============================================================================*/

int32_t AppNetworkService_DisconnectGsm(void)
{
    int32_t result = 0;

    if (!gNetworkService_state.gsmState.initialized) {
        return -1;
    }

    /* Close PPP connection via AppGsmManager */
    result = appGsmMngClosePPP();
    if (result != SUCCESS) {
        DEBUG_ERROR("->[E] AppNetworkService: appGsmMngClosePPP failed");
        priv_PublishNetworkEvent(0x03, 0, result); /* GSM_ERROR */
        return -2;
    }

    gNetworkService_state.gsmState.connected = false;
    priv_PublishNetworkEvent(0x02, 0, 0); /* GSM_DISCONNECTED */

    return 0;
}

/*==============================================================================
 * GSM Interface Status
 *============================================================================*/

int32_t AppNetworkService_IsGsmConnected(bool* isConnected)
{
    if (isConnected == NULL) {
        return -1;
    }

    *isConnected = gNetworkService_state.gsmState.connected;
    return 0;
}

/*** End Of File ***/
