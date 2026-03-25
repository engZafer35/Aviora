/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 11 Mar 2026 - 14:21:51
* #File Name    : EthConn_ENC28j60.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppGsmManager.h"

#include "net_config.h"

#include "AppGlobalVariables.h"
#include "AppTaskManager.h"
#include "AppInternalMsgFrame.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"

#include "AppNetworkService_Config.h"
#include "core/net.h"
#include "drivers/eth/enc28j60_driver.h"
#include "dhcp/dhcp_client.h"
#include "spi_driver.h"
#include "ext_int_driver.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef enum 
{
    ETH_CONN_STEP_INIT_ETH_DRIVER,   
    ETH_CONN_STEP_ETH_UP,
    ETH_CONN_STEP_ETH_DOWN,
    ETH_CONN_STEP_COMPLETED,
} EthModuleStep_t;

/********************************** VARIABLES *********************************/
static S32 gs_ethDbusID;
static S32 gs_timerId;

static EthModuleStep_t gs_ethInitStep = ETH_CONN_STEP_INIT_ETH_DRIVER;

/***************************** STATIC FUNCTIONS  ******************************/
static void ethPeriodicInfoCb (void)
{
    DBUS_PACKET dbPacket;
    
    DEBUG_INFO("Publishing Eth Data");

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_ETH;

    //1: link up, 0: link down
    (netInterface[ETH_INTERFACE_NUMBER].linkState == TRUE) ? (dbPacket.payload.data[0] = 1) : (dbPacket.payload.data[0] = 0);

    appDBusPublish(gs_ethDbusID, &dbPacket);
}

#if USE_CYCLONE_LIB == 1

void ethLinkChangeCallback(NetInterface *interface, bool_t linkState)
{
    ethPeriodicInfoCb(); // publish the link state change immediately
}

static RETURN_STATUS ethDown(NetInterface *interface)
{
    RETURN_STATUS retVal = FAILURE;

    if (NO_ERROR == netSetLinkState(interface, FALSE))
    {
        //todo: close driver
        retVal = SUCCESS;
    }
    return retVal;
}

static RETURN_STATUS ethUp(NetInterface *interface)
{
    RETURN_STATUS retVal = FAILURE;

    //todo: open driver

    if (NO_ERROR == netSetLinkState(interface, TRUE))
    {        
        retVal = SUCCESS;
    }
    return retVal;
}

//static PppSettings pppSettings;
//Ethernet interface configuration
#define APP_IF_NAME     "eth0"
#define APP_HOST_NAME   "aviora"
#define APP_MAC_ADDR    "02:03:04:05:06:07"

#define APP_IPV4_HOST_ADDR          "192.168.1.35"
#define APP_IPV4_SUBNET_MASK        "255.255.255.0"
#define APP_IPV4_DEFAULT_GATEWAY    "192.168.1.254"

static ethConnManagerTask(void* argument)
{    
    NetInterface *interface;
    error_t error;

    (void)argument;
    osDelayTask(1000);

    interface = &netInterface[ETH_INTERFACE_NUMBER];

    while(666)
    { 
        switch (gs_ethInitStep)
        {
            case ETH_CONN_STEP_INIT_ETH_DRIVER:
            {                
                //Set interface name
                netSetInterfaceName(interface, APP_IF_NAME);
                //Set host name
                netSetHostname(interface, APP_HOST_NAME);
                //Set host MAC address
                macStringToAddr(APP_MAC_ADDR, &macAddr);
                netSetMacAddr(interface, &macAddr);
                //Select the relevant network adapter

                netSetDriver(interface, &enc28j60Driver);
                netSetSpiDriver(interface, &spiDriver);
                netSetExtIntDriver(interface, &extIntDriver);

                //Initialize network interface
                if(NO_ERROR != netConfigInterface(interface))
                {                    
                    DEBUG_ERROR("Failed to configure interface %s!\r\n", interface->name);
                    APP_LOG_REC(g_sysLoggerID, "Failed to configure interface Ethernet!");
                    retVal = FAILURE;
                }

                if (SUCCESS == retVal)
                {
                    //Set IPv4 host address
                    ipv4StringToAddr(APP_IPV4_HOST_ADDR, &ipv4Addr);
                    ipv4SetHostAddr(interface, ipv4Addr);

                    //Set subnet mask
                    ipv4StringToAddr(APP_IPV4_SUBNET_MASK, &ipv4Addr);
                    ipv4SetSubnetMask(interface, ipv4Addr);

                    //Set default gateway
                    ipv4StringToAddr(APP_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
                    ipv4SetDefaultGateway(interface, ipv4Addr);
                }            
                
                if (SUCCESS == retVal)
                {
                    gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                    netAttachLinkChangeCallback(interface, ethLinkChangeCallback, NULL);

                    DEBUG_INFO("Eth driver initialized successfully");
                    APP_LOG_REC(g_sysLoggerID, "Eth driver initialized successfully");
                }
                break;      
            }

            case ETH_CONN_STEP_ETH_UP:
            {     
                if (SUCCESS == ethUp(interface))
                {
                    gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                    DEBUG_INFO("Ethernet connection established successfully!");
                    APP_LOG_REC(g_sysLoggerID, "Ethernet connection established successfully!");
                }    
                
                break;
            }
            
            case ETH_CONN_STEP_ETH_DOWN:
            {
                if (SUCCESS == ethDown(interface))
                {
                    gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                    DEBUG_INFO("eth connection down !");
                    APP_LOG_REC(g_sysLoggerID, "eth connection down !");                    
                }
                break;
            }

            default:
                break;
        }
        
        osDelayTask(1000);
    }
}

#else

static ethConnManagerTask(void* argument)
{      
    (void)argument;
    osDelayTask(1000);

    while(666)
    { 
        switch (gs_ethInitStep)
        {
            case ETH_CONN_STEP_INIT_ETH_DRIVER:
            {                
                gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                DEBUG_INFO("Eth driver initialized successfully");
                APP_LOG_REC(g_sysLoggerID, "Eth driver initialized successfully");
            
                break;      
            }

            case ETH_CONN_STEP_ETH_UP:
            {     
                gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                DEBUG_INFO("Ethernet connection established successfully!");
                APP_LOG_REC(g_sysLoggerID, "Ethernet connection established successfully!");
                                
                break;
            }
            
            case ETH_CONN_STEP_ETH_DOWN:
            {
                gs_ethInitStep = ETH_CONN_STEP_COMPLETED;
                DEBUG_INFO("eth connection down !");
                APP_LOG_REC(g_sysLoggerID, "eth connection down !");                    
            
                break;
            }

            default:
                break;
        }
        
        osDelayTask(1000);
    }
}

#endif
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS EthConn_Enc28J60_Init(void)
{
    RETURN_STATUS retVal = SUCCESS;
    ZOsTaskParameters tempParam;
    OsTaskId gsmTaskID;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    retVal = appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gs_ethDbusID);
    if (SUCCESS != retVal)
    {
        DEBUG_ERROR("Failed to register GSM manager to DBus!");
        APP_LOG_REC(g_sysLoggerID, "Failed to register GSM manager to DBus!");
        return FAILURE;
    }

    //return value of timer is not critical, so we do not check it
    middEventTimerRegister(&gs_timerId, ethPeriodicInfoCb, WAIT_1_MIN , TRUE);
    middEventTimerStart(gs_timerId);

    if (SUCCESS == retVal)
    {
        gsmTaskID = appTskMngCreate("GSM_TASK", ethConnManagerTask, NULL, &tempParam);

        if (OS_INVALID_TASK_ID != gsmTaskID)
        {
            DEBUG_ERROR("->[E] GSM Task created id: %d", gsmTaskID);
            APP_LOG_REC(g_sysLoggerID, "GSM: Task created successfully");
        }
        else
        {
            appDBusUnregister(gs_ethDbusID);
            retVal = FAILURE;

            DEBUG_ERROR("->[E] GSM Task could not be created");
            APP_LOG_REC(g_sysLoggerID, "GSM: Task could not be created");
        }
    }

    return retVal;
}

RETURN_STATUS EthConn_Enc28J60_CloseConn(void)
{    
    gs_ethInitStep = ETH_CONN_STEP_ETH_DOWN;
    return SUCCESS;
}

RETURN_STATUS EthConn_Enc28J60_Reconnect(void)
{    
    gs_ethInitStep = ETH_CONN_STEP_ETH_UP;
    return SUCCESS;
}

BOOL EthConn_Enc28J60IsNetworkReady(void)
{
    return netInterface[ETH_INTERFACE_NUMBER].linkState;
}

/******************************** End Of File *********************************/
