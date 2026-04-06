/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 2.0
* #Date         : April 3, 2026 - 00:05:26 AM
* #File Name    : MeterTcpComm.c
*******************************************************************************/
/******************************************************************************
* This file is used to manage the TCP connection for the gateway.

*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "MeterTcpComm.h"


#include "net_config.h"
/****************************** MACRO DEFINITIONS *****************************/
#define RET_SUCCESS    (0)
#define RET_FAILURE    (-1)

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdint.h>


static int g_sock = -1;

/*--------------------------------------------------*/
int meterTcpCommInit(void)
{
    struct sockaddr_in server_addr;
    struct timeval timeout;

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock < 0) 
    {        
        return -1;
    }

    /* Timeout (1 sec) */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(g_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) 
    {
        close(g_sock);
        return -1;
    }

    if (setsockopt(g_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) 
    {        
        close(g_sock);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(METER_TCP_PORT);

    if (inet_pton(AF_INET, METER_TCP_SERVER_IP, &server_addr.sin_addr) <= 0) 
    {        
        close(g_sock);
        return -1;
    }

    if (connect(g_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        close(g_sock);
        return -1;
    }

    return 0;
}

/*--------------------------------------------------*/
int meterTcpCommDeinit(void)
{
    if (g_sock >= 0) 
    {
        close(g_sock);
        g_sock = -1;
    }
    return 0;
}

/*--------------------------------------------------*/
int meterTcpCommSend(const U8 *data, U32 dataLeng, U32 timeout)
{
    if (g_sock < 0 || data == NULL)
        return -1;

    U32 total_sent = 0;

    while (total_sent < dataLeng) 
    {
        int ret = send(g_sock, data + total_sent, dataLeng - total_sent, 0);

        if (ret <= 0) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
             {
                printf("Send timeout\n");
            } 
            else
            {
                printf("send error");
            }
            return -1;
        }

        total_sent += ret;
    }

    return total_sent;
}

/*--------------------------------------------------*/
int meterTcpCommReceive(U8 *data, U32 *dataLeng, U32 timeout)
{
    if (g_sock < 0 || data == NULL || dataLeng == NULL)
        return -1;

    int ret = recv(g_sock, data, *dataLeng, 0);

    if (ret < 0) 
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            printf("Receive timeout (1 sn)\n");
        } else {
            printf("recv error");
        }
        return -1;
    }
    else if (ret == 0) 
    {
        printf("Connection closed by peer\n");
        return 0;
    }

    *dataLeng = ret;
    return ret;
}

int meterTcpCommSetBaudrate(U32 baudrate)
{
    return 0; // No-op for TCP communication
}
/******************************** End Of File *********************************/
