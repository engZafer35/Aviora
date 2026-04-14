/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Kingdom of Heaven
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 18:36:18
* #File Name    : AppCyclonePPPMng.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppCyclonePPPMng.h"
#include "../../Customers/NetworkService_Config.h"

#include "net_config.h"

#include "AppGlobalVariables.h"
#include "AppTaskManager.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef union
{
    struct
    {
        BOOL pppLinkState   : 1; //1: ppp link is ready, 0: ppp link is not ready
        BOOL modemState     : 1; //1:Modem is ready, 0 modem is not ready
        BOOL gsmSignalLevel : 7; //100 max signal quality, 0 no signal
    };

}CyclonePppDataBusPacket;

typedef enum 
{
    CYCLONE_PPP_MODULE_STEP_PPP_CONNECTION,
    CYCLONE_PPP_MODULE_STEP_INIT_PPP_MODEM,
    CYCLONE_PPP_MODULE_STEP_CONNECT_PPP,    
    CYCLONE_PPP_MODULE_STEP_DISCONNECTED,
    CYCLONE_PPP_MODULE_STEP_COMPLETED,
    CYCLONE_PPP_MODULE_STEP_GET_UPDATE_INFO, //update date, gsm signal level, ip, etc. 
} CyclonePppModuleStep_t;

/********************************** VARIABLES *********************************/
static S32 gs_cyclonePppDbusID;
static CyclonePppDataBusPacket gs_dataBusPck;
static S32 gs_cyclonePppTimerId;

static CyclonePppModuleStep_t gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_PPP_CONNECTION;

/***************************** STATIC FUNCTIONS  ******************************/
static void cyclonePppTimerCb (void)
{
    DBUS_PACKET dbPacket;
//    CyclonePppMsg cyclonePppMsg;

    DEBUG_INFO("Published Gsm Data");

//    gsmMsg.modemStat   = gs_dataBusPck.modemState;
//    gsmMsg.signalLevel = gs_dataBusPck.gsmSignalLevel;
//    gsmMsg.connStat    = gs_dataBusPck.pppLinkState;

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_GSM;

    dbPacket.payload.dataLeng = sizeof(gs_dataBusPck);
    memcpy(dbPacket.payload.data, &gs_dataBusPck, dbPacket.payload.dataLeng);

    appDBusPublish(gs_cyclonePppDbusID, &dbPacket);

    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_GET_UPDATE_INFO; // after publish cyclone ppp info, update cyclone ppp info in next timer period
}

RETURN_STATUS modemSendAtCommand(NetInterface *interface, const char *command, char *response, U32 size)
{
    RETURN_STATUS retVal = FAILURE;
    error_t error;
    size_t n;

    DEBUG_INFO("\r\nAT command:  %s", command);

    //Send AT command
    error = pppSendAtCommand(interface, command);

   //Check status code
   if(!error)
   {
      n = 0;
      while(n < size)
      {
         //Wait for a response from the modem
         error = pppReceiveAtCommand(interface, response + n, size - n);

         if(error)
         {
            break;
         }

         //Status string received?
         if(strstr(response, "OK") ||
            strstr(response, "CONNECT") ||
            strstr(response, "RING") ||
            strstr(response, "NO CARRIER") ||
            strstr(response, "ERROR") ||
            strstr(response, "NO ANSWER"))
         {
             DEBUG_INFO("AT response: %s", response);
             retVal = SUCCESS;
             break;
         }
         //Update the length of the response
         n = strlen(response);
      }
   }

   return retVal;
}

RETURN_STATUS modemInit(NetInterface *interface)
{
    char buffer[128];
//    ping(&netInterface[0], &ip, 32, 0xFF, 2000000, NULL);

    //Set timeout for blocking operations
    pppSetTimeout(interface, APP_PPP_TIMEOUT);

    //Debug message
    DEBUG_INFO("Reseting modem...");
    //todo: run hw reset function here

    DEBUG_INFO("Initializing modem..");

    //Module identification
    if(SUCCESS != modemSendAtCommand(interface, "AT+CGMM\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Get software version
    if(SUCCESS != modemSendAtCommand(interface, "AT+CGMR\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Enable verbose mode
    if(SUCCESS != modemSendAtCommand(interface, "AT+CMEE=2\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Set the functionality level (full fonctionnality)
    if(SUCCESS != modemSendAtCommand(interface, "AT+CFUN=1\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Query the ICCID of the SIM card
    if(SUCCESS != modemSendAtCommand(interface, "AT+QCCID\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Check if the SIM device needs the PIN code
    if(SUCCESS != modemSendAtCommand(interface, "AT+CPIN?\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Check whether the PIN code is required
    if(strstr(buffer, "+CPIN: SIM PIN") != NULL)
    {
#ifdef APP_PPP_PIN_CODE
      //Format AT+CPIN command
      sprintf(buffer, "AT+CPIN=%s\r", APP_PPP_PIN_CODE);

      //Send AT command
      if(NO_ERROR != modemSendAtCommand(interface, buffer, buffer, sizeof(buffer)))
      {
          return FAILURE;
      }
#else
       DEBUG_INFO("PIN code is required!");
       return FAILURE;
    #endif
    }
    else if(strstr(buffer, "+CPIN: READY") != NULL)
    {
        DEBUG_INFO("The PIN code is not required\n");
    }

    //Enable network registration
    if(SUCCESS != modemSendAtCommand(interface, "AT+CREG=2\r", buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

   //Wait for the module to be registered
   while(1)
   {
      //Check if the module is registered
      if(SUCCESS != modemSendAtCommand(interface, "AT+CREG?\r", buffer, sizeof(buffer)))
      {
          return FAILURE;
      }

      if(strstr(buffer, "+CREG: 2,0") != NULL)
      {
         //Not registered
      }
      else if(strstr(buffer, "+CREG: 2,1") != NULL)
      {
          //Registered (home network)
          break;
      }
      else if(strstr(buffer, "+CREG: 2,5") != NULL)
      {
          //Registered (roaming)
          break;
      }

       //Successful initialization
       osDelayTask(1000);
   }

   //Successful processing
   return SUCCESS;
}

RETURN_STATUS modemConnect(NetInterface *interface)
{
    char buffer[64];
    Ipv4Addr ipv4Addr;

    sprintf(buffer, "AT+CGDCONT=1,\"IP\",\"%s\"\r", APP_PPP_APN);

    if(SUCCESS != modemSendAtCommand(interface, buffer, buffer, sizeof(buffer)))
    {
       return FAILURE;
    }

    //Format ATD command
    sprintf(buffer, "ATD%s\r", APP_PPP_PHONE_NUMBER);

    if(SUCCESS != modemSendAtCommand(interface, buffer, buffer, sizeof(buffer)))
    {
        return FAILURE;
    }

    //Check response
    if(strstr(buffer, "NO CARRIER") != NULL)
    {
        return FAILURE;
    }
    else if(strstr(buffer, "CONNECT") == NULL)
    {
        return FAILURE;
    }

    //   //Clear local IPv4 address
    //   ipv4StringToAddr("0.0.0.0", &ipv4Addr);
    //   ipv4SetHostAddr(interface, ipv4Addr);
    //
    //   //Clear peer IPv4 address
    //   ipv4StringToAddr("0.0.0.0", &ipv4Addr);
    //   ipv4SetDefaultGateway(interface, ipv4Addr);

    //Set primary DNS server
    ipv4StringToAddr(APP_PPP_PRIMARY_DNS, &ipv4Addr);
    ipv4SetDnsServer(interface, 0, ipv4Addr);

    //Set secondary DNS server
    ipv4StringToAddr(APP_PPP_SECONDARY_DNS, &ipv4Addr);
    ipv4SetDnsServer(interface, 1, ipv4Addr);

    //Set username and password
    pppSetAuthInfo(interface, "", "");

    DEBUG_INFO("Establishing PPP connection...");

    //Establish a PPP connection
    if (NO_ERROR != pppConnect(interface))
    {
        return FAILURE;
    }

   return SUCCESS;
}

static void pppLinkStatusCb(NetInterface *interface, int linkState, void *param)
{
    DBUS_PACKET dbPacket;
    GsmMsg gsmMsg;

    //update link state and then share it over data bus
    gs_dataBusPck.pppLinkState = linkState;

    gsmMsg.modemStat    = gs_dataBusPck.modemState;
    gsmMsg.signalLevel  = gs_dataBusPck.gsmSignalLevel;
    gsmMsg.connStat     = gs_dataBusPck.pppLinkState;

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_GSM;

    appIntMsgCreateGsmMsg(&gsmMsg, dbPacket.payload.data, &dbPacket.payload.dataLeng);
    appDBusPublish(gs_cyclonePppDbusID, &dbPacket);
}

static RETURN_STATUS closePPP(void)
{
   RETURN_STATUS retVal = FAILURE;

   if (FALSE == isPPPWatingRxTxData())
   {
       DEBUG_INFO("Closing PPP connection...");
       APP_LOG_REC(g_sysLoggerID, "Closing PPP connection...");

       if (NO_ERROR == pppClose(&netInterface[PPP_INTERFACE_NUMBER]))
       {
           retVal = SUCCESS;
           gs_dataBusPck.pppLinkState = FALSE;
       }
   }
   return retVal;
}

static void updateGsmModemInfo(void)
{
    static int signalLevel = 50; //TODO: for test purpose, need to get real signal level from modem
    signalLevel += 3;
    if (signalLevel > 100)
    {
        signalLevel = 50;
    }

    gs_dataBusPck.gsmSignalLevel = signalLevel;
}

//static PppSettings pppSettings;
static PppContext pppContext;
#define APP_IF_NAME "PPP0"

static void cyclonePppManagerTask(void* argument)
{    
    NetInterface *interface;
    PppSettings pppSettings;

    (void)argument;
    osDelayTask(1000);

    //Configure the first network interface  
    interface = &netInterface[PPP_INTERFACE_NUMBER];

    while(666)
    { 
        switch (gs_cyclonePppInitStep)
        {
            case CYCLONE_PPP_MODULE_STEP_PPP_CONNECTION:
            {                              
                //Get default PPP settings
                pppGetDefaultSettings(&pppSettings);
                //Select the underlying interface
                pppSettings.interface = interface;
                //Default async control character map
                pppSettings.accm = 0x00000000;
                //Allowed authentication protocols
                pppSettings.authProtocol = PPP_AUTH_PROTOCOL_PAP | PPP_AUTH_PROTOCOL_CHAP_MD5;

                //Initialize PPP
                if(NO_ERROR != pppInit(&pppContext, &pppSettings))
                {
                    retVal = FAILURE;
                    DEBUG_ERROR("Failed to initialize PPP!");
                    APP_LOG_REC(g_sysLoggerID, "Failed to initialize PPP!");
                }

                if (SUCCESS == retVal)
                {
                    //Set interface name
                    netSetInterfaceName(interface, PPP_IF_NAME);
                    //Select the relevant UART driver
                    netSetUartDriver(interface, &uartDriver);

                    //Initialize network interface
                    if(NO_ERROR != netConfigInterface(interface))
                    {
                        retVal = FAILURE;
                        DEBUG_ERROR("Failed to configure interface %s!", interface->name);
                        APP_LOG_REC(g_sysLoggerID, "Failed to configure interface PPP!");
                    }
                }
                
                if (SUCCESS == retVal)
                {
                    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_INIT_PPP_MODEM;
                    DEBUG_INFO("PPP interface initialized successfully!");
                    APP_LOG_REC(g_sysLoggerID, "PPP interface initialized successfully!");
                }
                break;      
            }
            
            case CYCLONE_PPP_MODULE_STEP_INIT_PPP_MODEM:
            {
                if(SUCCESS == modemInit(interface))
                {
                    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_CONNECT_PPP;
                    gs_dataBusPck.modemState = TRUE;
                    netAttachLinkChangeCallback(interface, pppLinkStatusCb, NULL);

                    DEBUG_INFO("PPP modem initialized successfully!");
                    APP_LOG_REC(g_sysLoggerID, "PPP modem initialized successfully!");-                    
                }

                break;
            }

            case CYCLONE_PPP_MODULE_STEP_CONNECT_PPP:
            {         
                if (SUCCESS == modemConnect(interface))
                {
                    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_COMPLETED;
                    DEBUG_INFO("PPP connection established successfully!");
                    APP_LOG_REC(g_sysLoggerID, "PPP connection established successfully!");
                    gs_dataBusPck.pppLinkState = TRUE;

                    gs_dataBusPck.gsmSignalLevel = 80; //TODO: for test purpose

                    gsmTimerCb(); // publish the initial state immediately after connection
                }             
                break;
            }
            
            case CYCLONE_PPP_MODULE_STEP_DISCONNECTED:
            {
                if (SUCCESS == closePPP())
                {
                    DEBUG_INFO("PPP connection closed successfully!");
                    APP_LOG_REC(g_sysLoggerID, "PPP connection closed successfully!");

                    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_COMPLETED;
                }
                break;
            }

            case CYCLONE_PPP_MODULE_STEP_GET_UPDATE_INFO:
            {
                updateGsmModemInfo();
                gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_COMPLETED;
                break;
            }

            default:
                break;
        }
        
        osDelayTask(1000);
    }

}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appCyclonePppMngStart(void)
{
    RETURN_STATUS retVal = SUCCESS;
    ZOsTaskParameters tempParam;
    OsTaskId cyclonePppTaskID;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    retVal = appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gs_cyclonePppDbusID);
    if (SUCCESS != retVal)
    {
        DEBUG_ERROR("Failed to register Cyclone-PPP manager to DBus!");
        APP_LOG_REC(g_sysLoggerID, "Failed to register Cyclone-PPP manager to DBus!");
        return FAILURE;
    }

    //return value of timer is not critical, so we do not check it
    middEventTimerRegister(&gs_cyclonePppTimerId, cyclonePppTimerCb, WAIT_10_SEC , TRUE);
    middEventTimerStart(gs_cyclonePppTimerId);

    if (SUCCESS == retVal)
    {
        cyclonePppTaskID = appTskMngCreate("CYCLONE_PPP_TASK", cyclonePppManagerTask, NULL, &tempParam);

        if (OS_INVALID_TASK_ID != cyclonePppTaskID)
        {
            DEBUG_ERROR("->[E] CYCLONE_PPP Task created id: %d", cyclonePppTaskID);
            APP_LOG_REC(g_sysLoggerID, "CYCLONE_PPP: Task created successfully");
        }
        else
        {
            appDBusUnregister(gs_cyclonePppDbusID);
            retVal = FAILURE;

            DEBUG_ERROR("->[E] CYCLONE_PPP Task could not be created");
            APP_LOG_REC(g_sysLoggerID, "CYCLONE_PPP: Task could not be created");
        }
    }

    return retVal;
}

RETURN_STATUS appCyclonePppMngClose(void)
{    
    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_DISCONNECTED;
    return SUCCESS;
}

RETURN_STATUS appCyclonePppMngReconnect(void)
{    
    gs_cyclonePppInitStep = CYCLONE_PPP_MODULE_STEP_INIT_PPP_MODEM;
    return SUCCESS;
}

BOOL appCyclonePppMngIsNetworkReady(void)
{
    return gs_dataBusPck.pppLinkState;
}

/******************************** End Of File *********************************/