/*
 ============================================================================
 Name        : Z_PiPiPiPiPiP_Test.c
 Author      : Zafer
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "core/net.h"
#include "ppp/ppp.h"
#include "uart_driver.h"
#include "debug.h"
#include "ftp/ftp_client.h"
#include "core/socket.h"
#include "core/bsd_socket.h"

#include "modem.h"

//PPP interface configuration
#define APP_IF_NAME "ppp0"

//Application configuration
#define APP_FTP_SERVER_NAME "test.rebex.net"
#define APP_FTP_SERVER_PORT 21
#define APP_FTP_LOGIN "demo"
#define APP_FTP_PASSWORD "password"
#define APP_FTP_FILENAME "readme.txt"

//Global variables
PppSettings pppSettings;
PppContext pppContext;

FtpClientContext ftpClientContext;

error_t ftpClientTest(void)
{
   error_t error;
   size_t n;
   IpAddr ipAddr;
   char_t buffer[128];

   //Initialize FTP client context
   ftpClientInit(&ftpClientContext);

   //Start of exception handling block
   do
   {
      //Debug message
      TRACE_INFO("\r\n\r\nResolving server name...\r\n");

      //Resolve FTP server name
      error = gethostbyname(NULL, APP_FTP_SERVER_NAME, &ipAddr, 0);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to resolve server name!\r\n");
         break;
      }

      //Set timeout value for blocking operations
      error = ftpClientSetTimeout(&ftpClientContext, 20000);
      //Any error to report?
      if(error)
         break;

      //Debug message
      TRACE_INFO("Connecting to FTP server %s...\r\n",
         ipAddrToString(&ipAddr, NULL));

      //Connect to the FTP server
      error = ftpClientConnect(&ftpClientContext, &ipAddr, APP_FTP_SERVER_PORT,
         FTP_MODE_PLAINTEXT | FTP_MODE_PASSIVE);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to connect to FTP server!\r\n");
         break;
      }

      //Login to the FTP server using the provided username and password
      error = ftpClientLogin(&ftpClientContext, APP_FTP_LOGIN, APP_FTP_PASSWORD);
      //Any error to report?
      if(error)
         break;

      //Open the specified file for reading
      error = ftpClientOpenFile(&ftpClientContext, APP_FTP_FILENAME,
         FTP_FILE_MODE_READ | FTP_FILE_MODE_BINARY);
      //Any error to report?
      if(error)
         break;

      //Read the contents of the file
      while(!error)
      {
         //Read data
         error = ftpClientReadFile(&ftpClientContext, buffer, sizeof(buffer) - 1, &n, 0);

         //Check status code
         if(!error)
         {
            //Properly terminate the string with a NULL character
            buffer[n] = '\0';
            //Dump the contents of the file
            TRACE_INFO("%s", buffer);
         }
      }

      //Terminate the string with a line feed
      TRACE_INFO("\r\n");

      //Any error to report?
      if(error != ERROR_END_OF_STREAM)
         break;

      //Close file
      error = ftpClientCloseFile(&ftpClientContext);
      //Any error to report?
      if(error)
         break;

      //Gracefully disconnect from the FTP server
      ftpClientDisconnect(&ftpClientContext);

      //Debug message
      TRACE_INFO("Connection closed\r\n");

      //End of exception handling block
   } while(0);

   //Release FTP client context
   ftpClientDeinit(&ftpClientContext);

   //Return status code
   return error;
}
#if 0
static int createUdpSockets(void)
{
    int retVal = 0;
    struct sockaddr_in serverAddr;

    g_UdpSocketFd = c_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(g_UdpSocketFd >= 0)
    {
        serverAddr.sin_family        = AF_INET;
        serverAddr.sin_port          = htons(UDP_SERVER_PORT_NUM);
        serverAddr.sin_addr.s_addr   = c_inet_addr(APP_IPV4_HOST_ADDR);

        // Bind the socket with the server address
        if (c_bind(g_UdpSocketFd, (const struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) < 0 )
        {
            retVal = FAILURE;
        }

        if (SUCCESS == retVal)
        {
            DEBUG_INFO("[I]-> UDP Server created !!!");
            DEBUG_INFO("[I]-> UDP Server IP: %s", APP_IPV4_HOST_ADDR);
            DEBUG_INFO("[I]-> UDP Server Port: %d", UDP_SERVER_PORT_NUM);
        }
    }
    else
    {
        retVal = FAILURE;
        DEBUG_ERROR("[E]-> UDP Socket could not be created");
    }

    return retVal;
}
#endif
void pppTestTask(void *param)
{
   error_t error;
   bool_t initialized;
   NetInterface *interface;

   //The modem is not yet initialized
   initialized = FALSE;

   //Point to the PPP network interface
   interface = &netInterface[0];

   //Modem initialization is performed only once
   if(!initialized)
   {
       //Modem initialization
       error = modemInit(interface);

       //Check status code
       if(error)
       {
          //Debug message
          TRACE_WARNING("Modem initialization failed!\r\n");
       }
       else
       {
          //Successful initialization
          initialized = TRUE;
       }
    }
    else
    {
        //Modem is already initialized
        error = NO_ERROR;
    }

    //Check status code
    if(!error)
    {
        //Establish PPP connection
        //         connectTCP(interface);
        error = modemConnect(interface);

        //Check status code
        if(error)
        {
           //Debug message
           TRACE_WARNING("Failed to established PPP connection!\r\n");
        }
        else
        {
           //FTP client test routine
//           ftpClientTest();
           //Close PPP connetion
           //modemDisconnect(interface);
        }
    }
}

#define PORT 5566
#define SA struct sockaddr
static pthread_t gs_tcpThread;
int connfd;
void func(void)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = { 0 };

    sleep(1);

    // Creating socket file descriptor
    if ((server_fd = SOCKET(AF_INET, SOCK_STREAM, 0)) < 0) {
       perror("socket failed");
       exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (SETSOCKOPT(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)))
    {
       perror("setsockopt");
       exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INET_ADDR("5.11.182.122");;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (BIND(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
       perror("bind failed");
       exit(EXIT_FAILURE);
    }
    if (LISTEN(server_fd, 3) < 0) {
       perror("listen");
       exit(EXIT_FAILURE);
    }
    if ((new_socket = ACCEPT(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
       perror("accept");
       exit(EXIT_FAILURE);
    }

    int c = 0;
    while(1)
    {
        bzero(buffer, 1024);
        RECV(new_socket, buffer, 1024 - 1, 0);
        printf("%s\n", buffer);

        usleep(100000);

        bzero(buffer, 1024);
        sprintf(buffer, "%d-Serverr ulaan %d-Serverr ulaan %d-Serverr ulaan", c++,c,c);
        SEND(new_socket, buffer, strlen(buffer), 0);
    }
    // closing the connected socket
    // After chatting close the socket
//    close(new_socket);
//    // closing the listening socket
//    close(server_fd);

}

void createTCPzafer(void)
{
    //Create a new thread
    if(0 != pthread_create(&gs_tcpThread, NULL, func, NULL))
    {
        gs_tcpThread = OS_INVALID_TASK_ID;
    }
}
void debugInit(uint32_t baudrate)
{

}
int main(void)
{
    error_t error;
    NetInterface *interface;

    //Initialize kernel
    osInitKernel();
    //Configure debug UART
    debugInit(115200);

    //Start-up message
    TRACE_INFO("\r\n");
    TRACE_INFO("**********************************\r\n");
    TRACE_INFO("*** CycloneTCP FTP Client Demo ***\r\n");
    TRACE_INFO("**********************************\r\n");
    TRACE_INFO("Copyright: 2010-2024 Oryx Embedded SARL\r\n");
    TRACE_INFO("Compiled: %s %s\r\n", __DATE__, __TIME__);
    TRACE_INFO("Target: Linux-Ubuntu\r\n");
    TRACE_INFO("\r\n");

    //TCP/IP stack initialization
    error = netInit();
    //Any error to report?
    if(error)
    {
       //Debug message
       TRACE_ERROR("Failed to initialize TCP/IP stack!\r\n");
    }

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
    error = pppInit(&pppContext, &pppSettings);
    //Any error to report?
    if(error)
    {
       //Debug message
       TRACE_ERROR("Failed to initialize PPP!\r\n");
    }

    //Set interface name
    netSetInterfaceName(interface, APP_IF_NAME);
    //Select the relevant UART driver
    netSetUartDriver(interface, &uartDriver);


    //Initialize network interface
    error = netConfigInterface(interface);
    //Any error to report?
    if(error)
    {
       //Debug message
       TRACE_ERROR("Failed to configure interface %s!\r\n", interface->name);
    }

    sleep(1);
    pppTestTask(NULL);
    sleep(1);
    createTCPzafer();

    IpAddr ip;
    ip.length = sizeof(Ipv4Addr);
    ip.ipv4Addr  = IPV4_ADDR(194,108, 117, 16);
    error_t err = ERROR_FAILURE;
    char buff[16] = "";

    ipv4AddrToString(ip.ipv4Addr, buff);
    int c = 0;
    while("zafer")
    {
//        err = ping(&netInterface[0], &ip, 32, 0xFF, 2000000, NULL);
//        if(!err)
//            printf("%d- Ping send to %s \n",c++, buff);
//        else
//            printf("Ping Error - %d !! \n", err);
        sleep(1);
    }


    return EXIT_SUCCESS;
}
