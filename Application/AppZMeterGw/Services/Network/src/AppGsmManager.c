/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 11 Mar 2026 - 14:21:51
* #File Name    : AppGsmManager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define DEBUG_LEVEL  (DEBUG_LEVEL_DEBUG)
/********************************* INCLUDES ***********************************/
#include "AppGsmManager.h"

#include "net_config.h"
#include "uart_driver.h"

#include "AppInternalMsgFrame.h"
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

}GsmDataBusPacket;

typedef enum 
{
    GSM_MODULE_STEP_PPP_CONNECTION,
    GSM_MODULE_STEP_INIT_GSM_MODEM,
    GSM_MODULE_STEP_CONNECT_PPP,    
    GSM_MODULE_STEP_DISCONNECTED,
    GSM_MODULE_STEP_COMPLETED,
    GSM_MODULE_STEP_GET_UPDATE_INFO, //update date, gsm signal level, ip, etc. 
} GsmModuleStep_t;

/********************************** VARIABLES *********************************/
static S32 gs_gsmDbusID;
static GsmDataBusPacket gs_dataBusPck;
static S32 gs_timerId;

static GsmModuleStep_t gs_smInitStep = GSM_MODULE_STEP_PPP_CONNECTION;

/***************************** STATIC FUNCTIONS  ******************************/
static void gsmTimerCb (void)
{
    DBUS_PACKET dbPacket;
    GsmMsg gsmMsg;

    DEBUG_INFO("Published Gsm Data");

    gsmMsg.modemStat   = gs_dataBusPck.modemState;
    gsmMsg.signalLevel = gs_dataBusPck.gsmSignalLevel;
    gsmMsg.connStat    = gs_dataBusPck.pppLinkState;

    dbPacket.packetID   = 0;
    dbPacket.pri        = EN_PRIORITY_MED;
    dbPacket.retainFlag = TRUE;
    dbPacket.topic      = EN_DBUS_TOPIC_GSM;

    appIntMsgCreateGsmMsg(&gsmMsg, dbPacket.payload.data, &dbPacket.payload.dataLeng);
    appDBusPublish(gs_gsmDbusID, &dbPacket);

    gs_smInitStep = GSM_MODULE_STEP_GET_UPDATE_INFO; // after publish gsm info, update gsm info in next timer period
}

#if USE_CYCLONE_LIB == 1

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
    appDBusPublish(gs_gsmDbusID, &dbPacket);
}

static RETURN_STATUS closePPP(void)
{
   RETURN_STATUS retVal = FAILURE;

   if (FALSE == isPPPWatingRxTxData())
   {
       DEBUG_INFO("Closing PPP connection...");

       if (NO_ERROR == pppClose(&netInterface[0]))
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

static gsmConnManagerTask(void* argument)
{    
    NetInterface *interface;
    PppSettings pppSettings;

    (void)argument;
    osDelayTask(1000);

    while(666)
    { 
        switch (gs_smInitStep)
        {
            case GSM_MODULE_STEP_PPP_CONNECTION:
            {
                //Configure the first network interface
                interface = &netInterface[0];

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
                    appLogRec(g_sysLoggerID, "Failed to initialize PPP!");
                }

                if (SUCCESS == retVal)
                {
                    //Set interface name
                    netSetInterfaceName(interface, APP_IF_NAME);
                    //Select the relevant UART driver
                    netSetUartDriver(interface, &uartDriver);

                    //Initialize network interface
                    if(NO_ERROR != netConfigInterface(interface))
                    {
                        retVal = FAILURE;
                        DEBUG_ERROR("Failed to configure interface %s!", interface->name);
                        appLogRec(g_sysLoggerID, "Failed to configure interface Gsm!");
                    }
                }
                
                if (SUCCESS == retVal)
                {
                    gs_smInitStep = GSM_MODULE_STEP_INIT_GSM_MODEM;
                    DEBUG_INFO("PPP interface initialized successfully!");
                    appLogRec(g_sysLoggerID, "PPP interface initialized successfully!");
                }
                break;      
            }
            
            case GSM_MODULE_STEP_INIT_GSM_MODEM:
            {
                if(SUCCESS == modemInit(interface))
                {
                    gs_smInitStep = GSM_MODULE_STEP_CONNECT_PPP;
                    gs_dataBusPck.modemState = TRUE;
                    netAttachLinkChangeCallback(&netInterface[0], pppLinkStatusCb, NULL);

                    DEBUG_INFO("GSM modem initialized successfully!");
                    appLogRec(g_sysLoggerID, "GSM modem initialized successfully!");-                    
                }

                break;
            }

            case GSM_MODULE_STEP_CONNECT_PPP:
            {         
                if (SUCCESS == modemConnect(&netInterface[0]))
                {
                    gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
                    DEBUG_INFO("PPP connection established successfully!");
                    appLogRec(g_sysLoggerID, "PPP connection established successfully!");
                    gs_dataBusPck.pppLinkState = TRUE;

                    gs_dataBusPck.gsmSignalLevel = 80; //TODO: for test purpose

                    gsmTimerCb(); // publish the initial state immediately after connection
                }             
                break;
            }
            
            case GSM_MODULE_STEP_DISCONNECTED:
            {
                if (SUCCESS == closePPP())
                {
                    DEBUG_INFO("PPP connection closed successfully!");
                    appLogRec(g_sysLoggerID, "PPP connection closed successfully!");

                    gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
                }
                break;
            }

            case GSM_MODULE_STEP_GET_UPDATE_INFO:
            {
                updateGsmModemInfo();
                gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
                break;
            }

            default:
                break;
        }
        
        osDelayTask(1000);
    }

}

#else
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

static gsmConnManagerTask(void* argument)
{    
    NetInterface *interface;
    PppSettings pppSettings;

    (void)argument;
    osDelayTask(1000);

    while(666)
    { 
        switch (gs_smInitStep)
        {
            case GSM_MODULE_STEP_PPP_CONNECTION:
            {                
                gs_smInitStep = GSM_MODULE_STEP_INIT_GSM_MODEM;
                DEBUG_INFO("PPP interface initialized successfully!");
                appLogRec(g_sysLoggerID, "PPP interface initialized successfully!");
            
                break;      
            }
            
            case GSM_MODULE_STEP_INIT_GSM_MODEM:
            {
                gs_smInitStep = GSM_MODULE_STEP_CONNECT_PPP;
                gs_dataBusPck.modemState = TRUE;

                DEBUG_INFO("GSM modem initialized successfully!");
                appLogRec(g_sysLoggerID, "GSM modem initialized successfully!");-                    
            
                break;
            }

            case GSM_MODULE_STEP_CONNECT_PPP:
            {         
                gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
                DEBUG_INFO("PPP connection established successfully!");
                appLogRec(g_sysLoggerID, "PPP connection established successfully!");
                gs_dataBusPck.pppLinkState = TRUE;

                gs_dataBusPck.gsmSignalLevel = 80; //TODO: for test purpose

                gsmTimerCb(); // publish the initial state immediately after connection
                    
                break;
            }
            
            case GSM_MODULE_STEP_DISCONNECTED:
            {
                DEBUG_INFO("PPP connection closed successfully!");
                appLogRec(g_sysLoggerID, "PPP connection closed successfully!");

                gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
                
                break;
            }

            case GSM_MODULE_STEP_GET_UPDATE_INFO:
            {
                updateGsmModemInfo();
                gs_smInitStep = GSM_MODULE_STEP_COMPLETED;
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
RETURN_STATUS appGsmMngInit(void)
{
    RETURN_STATUS retVal = SUCCESS;
    ZOsTaskParameters tempParam;
    OsTaskId gsmTaskID;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    retVal = appDBusRegister(EN_DBUS_TOPIC_DEVICE, &gs_gsmDbusID);
    if (SUCCESS != retVal)
    {
        DEBUG_ERROR("Failed to register GSM manager to DBus!");
        appLogRec(g_sysLoggerID, "Failed to register GSM manager to DBus!");
        return FAILURE;
    }

    //return value of timer is not critical, so we do not check it
    middEventTimerRegister(&gs_timerId, gsmTimerCb, WAIT_10_SEC , TRUE);
    middEventTimerStart(gs_timerId);

    if (SUCCESS == retVal)
    {
        gsmTaskID = appTskMngCreate("GSM_TASK", gsmConnManagerTask, NULL, &tempParam);

        if (OS_INVALID_TASK_ID != gsmTaskID)
        {
            DEBUG_ERROR("->[E] GSM Task created id: %d", gsmTaskID);
            appLogRec(g_sysLoggerID, "GSM: Task created successfully");
        }
        else
        {
            DEBUG_ERROR("->[E] GSM Task could not be created");
            appLogRec(g_sysLoggerID, "GSM: Task could not be created");
            retVal = FAILURE;
        }
    }

    return retVal;
}

RETURN_STATUS appGsmMngCloseConn(void)
{    
    gs_smInitStep = GSM_MODULE_STEP_DISCONNECTED;
    return SUCCESS;
}

RETURN_STATUS appGsmMngReconnect(void)
{    
    gs_smInitStep = GSM_MODULE_STEP_INIT_GSM_MODEM;
    return SUCCESS;
}

BOOL appGsmMngIsNetworkReady(void)
{
    return gs_dataBusPck.pppLinkState;
}

/******************************** End Of File *********************************/
