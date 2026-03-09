/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jun 9, 2024 - 7:27:48 PM
* #File Name    : modem.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
#include "core/net.h"
#include "ppp/ppp.h"
#include "modem.h"
#include "debug.h"

//Application configuration
//#define APP_PPP_PIN_CODE "0000"
#define APP_PPP_APN "mgbs"
#define APP_PPP_PHONE_NUMBER "*99#"
#define APP_PPP_PRIMARY_DNS "8.8.8.8"
#define APP_PPP_SECONDARY_DNS "8.8.4.4"
#define APP_PPP_TIMEOUT 10000


/**
 * @brief Modem initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t modemInit(NetInterface *interface)
{
   error_t error;
   char_t buffer[128];

   //Set timeout for blocking operations
   pppSetTimeout(interface, APP_PPP_TIMEOUT);

   //Debug message
   TRACE_INFO("Reseting modem...\r\n");

   //Debug message
   TRACE_INFO("Initializing modem...\r\n");

   //Module identification
   error = modemSendAtCommand(interface, "AT+CGMM\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Get software version
   error = modemSendAtCommand(interface, "AT+CGMR\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Enable verbose mode
   error = modemSendAtCommand(interface, "AT+CMEE=2\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

//   //Enable hardware flow control
//   error = modemSendAtCommand(interface, "AT+IFC=2,2\r", buffer, sizeof(buffer));
//   //Any error to report?
//   if(error)
//      return error;

   //Set the functionality level (full fonctionnality)
   error = modemSendAtCommand(interface, "AT+CFUN=1\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Configure RAT searching sequence (LTE Cat M1 -> LTE Cat NB1 -> GSM)
//   error = modemSendAtCommand(interface, "AT+QCFG=\"nwscanseq\",020301\r", buffer, sizeof(buffer));
//   //Any error to report?
//   if(error)
//      return error;
//
//   //Configure RATs allowed to be searched (LTE only)
//   error = modemSendAtCommand(interface, "AT+QCFG=\"nwscanmode\",3\r", buffer, sizeof(buffer));
//   //Any error to report?
//   if(error)
//      return error;

   //Query the ICCID of the SIM card
   error = modemSendAtCommand(interface, "AT+QCCID\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Check if the SIM device needs the PIN code
   error = modemSendAtCommand(interface, "AT+CPIN?\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Check whether the PIN code is required
   if(strstr(buffer, "+CPIN: SIM PIN") != NULL)
   {
#ifdef APP_PPP_PIN_CODE
      //Format AT+CPIN command
      sprintf(buffer, "AT+CPIN=%s\r", APP_PPP_PIN_CODE);

      //Send AT command
      error = modemSendAtCommand(interface, buffer, buffer, sizeof(buffer));
      //Any error to report?
      if(error)
         return ERROR_INVALID_PIN_CODE;
#else
      //Debug message
      TRACE_ERROR("PIN code is required!\r\n");
      //Report an error
      return ERROR_FAILURE;
#endif
   }
   else if(strstr(buffer, "+CPIN: READY") != NULL)
   {
      //The PIN code is not required
       printf("The PIN code is not required\n");
   }

   //Enable network registration
   error = modemSendAtCommand(interface, "AT+CREG=2\r", buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Wait for the module to be registered
   while(1)
   {
      //Check if the module is registered
      error = modemSendAtCommand(interface, "AT+CREG?\r", buffer, sizeof(buffer));
      //Any error to report?
      if(error)
         return error;

      //Check registration status
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
   return NO_ERROR;
}


/**
 * @brief Establish a PPP session
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t connectTCP(NetInterface *interface)
{
    error_t error;
    char_t buffer[64];
    sprintf(buffer, "AT+QICSGP=1,1,\"mgbs\",\"\",\"\"\r");

    error = modemSendAtCommand(interface, buffer, buffer, sizeof(buffer));
    //Any error to report?
    if(error)
       return error;

    error = modemSendAtCommand(interface, "AT+QIACT?\r", buffer, sizeof(buffer));
    //Any error to report?
    if(error)
       return error;

    error = modemSendAtCommand(interface, "AT+QIACT=1\r", buffer, sizeof(buffer));
    //Any error to report?
    if(error)
       return error;


}

error_t modemConnect(NetInterface *interface)
{
   error_t error;
   char_t buffer[64];
   Ipv4Addr ipv4Addr;

   //Format AT+CGDCONT command
   sprintf(buffer, "AT+CGDCONT=1,\"IP\",\"%s\"\r", APP_PPP_APN);

   //Send AT command
   error = modemSendAtCommand(interface, buffer, buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Format ATD command
   sprintf(buffer, "ATD%s\r", APP_PPP_PHONE_NUMBER);

   //Send AT command
   modemSendAtCommand(interface, buffer, buffer, sizeof(buffer));
   //Any error to report?
   if(error)
      return error;

   //Check response
   if(strstr(buffer, "NO CARRIER") != NULL)
   {
      //Report an error
      return ERROR_NO_CARRIER;
   }
   else if(strstr(buffer, "CONNECT") == NULL)
   {
      //Report an error
      return ERROR_FAILURE;
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

   //Debug message
   TRACE_INFO("Establishing PPP connection...\r\n");

   //Establish a PPP connection
   error = pppConnect(interface);
   //Any error to report?
   if(error)
      return error;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Close a PPP session
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t modemDisconnect(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Closing PPP connection...\r\n");

   //Close the PPP connection
   error = pppClose(interface);

   //Return error code
   return error;
}


/**
 * @brief Send an AT command to the modem
 * @param[in] interface Underlying network interface
 * @param[in] command AT command
 * @param[in] response Pointer to the buffer where to copy the modem's response
 * @param[in] size Size of the response buffer
 * @return Error code
 **/

error_t modemSendAtCommand(NetInterface *interface,
   const char_t *command, char_t *response, size_t size)
{
   error_t error;
   size_t n;

   //Debug message
   TRACE_INFO("\r\nAT command:  %s\r\n", command);

   //Send AT command
   error = pppSendAtCommand(interface, command);

   //Check status code
   if(!error)
   {
      //Size of the response buffer
      n = 0;

      //Loop as long as necessary
      while(n < size)
      {
         //Wait for a response from the modem
         error = pppReceiveAtCommand(interface, response + n, size - n);

         //Check status code
         if(error)
         {
            //Exit immediately
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
            //Debug message
            TRACE_INFO("AT response: %s\r\n", response);
            //Exit immediately
            break;
         }

         //Update the length of the response
         n = strlen(response);
      }
   }

   //Return status code
   return error;
}
/******************************** End Of File *********************************/
