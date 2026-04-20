/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Kingdom of Heaven
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 17:29:12
* #File Name    : AppCycloneEthMng.h
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppCycloneEthMng.h"
#include "../../Customers/NetworkService_Config.h"

#include "net_config.h"
#include <CycloneTcp/cyclone_tcp/drivers/eth/enc28j60_driver.h>
#include "../../Driver/DeviceDrivers/ENC28J60/inc/spi_driver.h"
#include "../../Driver/DeviceDrivers/ENC28J60/inc/ext_int_driver.h"

#include "AppGlobalVariables.h"
#include "AppTaskManager.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef enum 
{
    CYCLONE_ETH_CONN_STEP_INIT_ETH_DRIVER,   
    CYCLONE_ETH_CONN_STEP_ETH_UP,
    CYCLONE_ETH_CONN_STEP_ETH_DOWN,
    CYCLONE_ETH_CONN_STEP_COMPLETED,
} CycloneEthModuleStep_t;
/********************************** VARIABLES *********************************/
static S32 gs_cycloneEthDbusID;
static S32 gs_cycloneEthTimerId;
OsTaskId gs_cycloneEthTaskID;

static CycloneEthModuleStep_t gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_INIT_ETH_DRIVER;

/***************************** STATIC FUNCTIONS  ******************************/
static void cycloneEthPeriodicInfoCb (NetInterface *interface, bool_t linkState)
{
    DBUS_PACKET dbPacket;
    
    DEBUG_INFO("Publishing Eth Data %s - link state %d", interface->name, linkState);

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_ETH;

    //1: link up, 0: link down
    dbPacket.payload.data[0] = netInterface[ETH_INTERFACE_NUMBER].linkState;

    if (netInterface[ETH_INTERFACE_NUMBER].linkState)
    {

        zosEventGroupSet(gp_systemSetupEventGrp, NETWORK_SERVICE_READY_FLAG);
    }

    //appDBusPublish(gs_cycloneEthDbusID, &dbPacket);
}

static void cycloneEthLinkChangeCallback(NetInterface *interface, bool_t linkState, void *param)
{
    cycloneEthPeriodicInfoCb(interface, linkState); // publish the link state change immediately
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

static void cycloneEthManagerTask(void* argument)
{    
    NetInterface *interface;
    MacAddr macAddr;
    Ipv4Addr ipv4Addr;
    error_t error;

    (void)argument;
    zosDelayTask(1000);

    interface = &netInterface[ETH_INTERFACE_NUMBER];

    while(666)
    { 
        switch (gs_cycloneEthInitStep)
        {
            case CYCLONE_ETH_CONN_STEP_INIT_ETH_DRIVER:
            {
                RETURN_STATUS retVal = SUCCESS;
                //Set interface name
                       netSetInterfaceName(interface, ETH_IF_NAME);
                       //Set host name
                       netSetHostname(interface, ETH_HOST_NAME);
                       //Set host MAC address
                       macStringToAddr(ETH_MAC_ADDR, &macAddr);
                       netSetMacAddr(interface, &macAddr);
                       //Select the relevant network adapter

                       netSetDriver(interface, &enc28j60Driver);
                       netSetSpiDriver(interface, &spiDriver);
                       netSetExtIntDriver(interface, &extIntDriver);

                       //Initialize network interface
                       error = netConfigInterface(interface);
                       //Any error to report?
                       if(error)
                       {
                           DEBUG_ERROR("Failed to configure interface %s!\r\n", interface->name);
                           retVal = FAILURE;
                       }

                       if (SUCCESS == retVal)
                       {
                           //Set IPv4 host address
                           ipv4StringToAddr(ETH_IP_ADDR, &ipv4Addr);
                           ipv4SetHostAddr(interface, ipv4Addr);

                           //Set subnet mask
                           ipv4StringToAddr(ETH_SUBNET_MASK, &ipv4Addr);
                           ipv4SetSubnetMask(interface, ipv4Addr);

                           //Set default gateway
                           ipv4StringToAddr(ETH_DEFAULT_GATEWAY, &ipv4Addr);
                           ipv4SetDefaultGateway(interface, ipv4Addr);
                       }

                
                if (SUCCESS == retVal)
                {
                    gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_COMPLETED;
                    netAttachLinkChangeCallback(interface, cycloneEthLinkChangeCallback, NULL);

                    DEBUG_INFO("Cyclone-Eth driver initialized successfully");
                    APP_LOG_REC(g_sysLoggerID, "Cyclone-Eth driver initialized successfully");
                }
                break;      
            }

            case CYCLONE_ETH_CONN_STEP_ETH_UP:
            {     
                if (SUCCESS == ethUp(interface))
                {
                    gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_COMPLETED;
                    DEBUG_INFO("Cyclone-Ethernet connection established successfully!");
                    APP_LOG_REC(g_sysLoggerID, "Cyclone-Ethernet connection established successfully!");
                }    
                
                break;
            }
            
            case CYCLONE_ETH_CONN_STEP_ETH_DOWN:
            {
                if (SUCCESS == ethDown(interface))
                {
                    gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_COMPLETED;
                    DEBUG_INFO("Cyclone-ETH connection down !");
                    APP_LOG_REC(g_sysLoggerID, "Cyclone-ETH connection down !");                    
                }
                break;
            }

            default:
                break;
        }
        
        zosDelayTask(1000);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appCycloneEthMngStart(void)
{
    RETURN_STATUS retVal = SUCCESS;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE*5;

    retVal = appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gs_cycloneEthDbusID);
    if (SUCCESS != retVal)
    {
        DEBUG_ERROR("Failed to register Cyclone-ETH manager to DBus!");
        APP_LOG_REC(g_sysLoggerID, "Failed to register Cyclone-ETH manager to DBus!");
        return FAILURE;
    }

    //return value of timer is not critical, so we do not check it
//    middEventTimerRegister(&gs_cycloneEthTimerId, cycloneEthPeriodicInfoCb, WAIT_1_MIN , TRUE);
//    middEventTimerStart(gs_cycloneEthTimerId);

    if (SUCCESS == retVal)
    {
        gs_cycloneEthTaskID = appTskMngCreate("CYCLONE_ETH_TASK", cycloneEthManagerTask, NULL, &tempParam);

        if (OS_INVALID_TASK_ID != gs_cycloneEthTaskID)
        {
            DEBUG_ERROR("->[E] CYCLONE_ETH Task created id: %lu", gs_cycloneEthTaskID);
            APP_LOG_REC(g_sysLoggerID, "CYCLONE_ETH: Task created successfully");
        }
        else
        {
            appDBusUnregister(gs_cycloneEthDbusID);
            retVal = FAILURE;

            DEBUG_ERROR("->[E] CYCLONE_ETH Task could not be created");
            APP_LOG_REC(g_sysLoggerID, "CYCLONE_ETH: Task could not be created");
        }
    }

    return retVal;
}

RETURN_STATUS appCycloneEthMngClose(void)
{    
    gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_ETH_DOWN;
    return SUCCESS;
}

RETURN_STATUS appCycloneEthMngReconnect(void)
{    
    gs_cycloneEthInitStep = CYCLONE_ETH_CONN_STEP_ETH_UP;
    return SUCCESS;
}

BOOL appCycloneEthMngIsNetworkReady(void)
{
    return netInterface[ETH_INTERFACE_NUMBER].linkState;
}

/******************************** End Of File *********************************/
