/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 6, 2024 - 1:42:21 PM
* #File Name    : AppMeterMessageHandler.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppMeterMessageHandler.h"
#include "MiddSerialComm.h"

#include "DrvLinuxUartGeneric.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
char meterReplyBuff[2048] = "";
struct UartDevice myUart;

pthread_mutex_t meterMutex;
/***************************** STATIC FUNCTIONS  ******************************/
void txCmpletedCb(int indx)
{
    if (1 == indx)
    {

    }
    else if (2 == indx)
    {

    }
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appMeterMsgHandlerSetSerialInterface(MeterSerialInterface *meterInt)
{
    RETURN_STATUS retVal = SUCCESS;
    myUart.filename = "/dev/ttyUSB1";
    myUart.rate = B300;

    if (0 != uart_start(&myUart, 0))
    {
        retVal = FAILURE;
        printf("---- Uart driver cannot be initialized \n");
    }

    pthread_mutex_init(&meterMutex, NULL);
    return retVal;
}

RETURN_STATUS appMeterAddMeter(MeterTypes_t type, MeterBrands_t brand, int meterSerialNum)
{
    DEBUG_INFO("->[I] Meter Type: %d ", type);
    DEBUG_INFO("->[I] Meter Brand %d ", brand);


    return SUCCESS;
}

RETURN_STATUS appMeterMsgHandler(const Msg_Handler_Message *message, U8 *replyMsg, U32 *replyMsgLeng)
{
    pthread_mutex_lock(&meterMutex);
    char buff[] = {0x2F, 0x3F, 0x31 , 0x31 , 0x31 , 0x31 , 0x31 , 0x31 , 0x31 , 0x33, 0x21, 0x0D, 0x0A};
    char routbuff[] = {0x06, 0x30, 0x30, 0x30, 0x0D, 0x0A};
    char rbuf[1024] = "";
    int rindex = 0;

    DEBUG_INFO("->[I] meter message handler ");
//
//    strcpy(replyMsg, "Meter Message Handler");
//    *replyMsgLeng = strlen("Meter Message Handler") +1;
//
//    middSerialCommSendDMA(EN_SERIAL_LINE_1, meterReplyBuff, sizeof(meterReplyBuff), txCmpletedCb);

            int ret = uart_writen(&myUart, buff, sizeof(buff));

            sleep(2);
            do
            {
                 ret = uart_reads(&myUart, rbuf, sizeof(rbuf));

            }while(ret > 0);

            sleep(1);
            ret = uart_writen(&myUart, routbuff, sizeof(routbuff)); //send readout request

            sleep(1);
            do
            {
                 ret = uart_reads(&myUart, rbuf+rindex, sizeof(rbuf));
                 if (ret > 0)
                     rindex += ret;

                 sleep(1);
            }while(ret > 0);

            printf("%s", rbuf);

            memcpy(replyMsg, rbuf, rindex);
            *replyMsgLeng = rindex;

    sleep(5);
    pthread_mutex_unlock(&meterMutex);
    return SUCCESS;
}


/******************************** End Of File *********************************/
