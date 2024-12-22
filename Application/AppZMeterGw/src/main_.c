/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 22 Mar 2024 - 11:16:23
* #File Name    : main.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (1)
/********************************* INCLUDES ***********************************/
#include "Project_Conf.h"
#include "AppZMeterGw.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>


#include "Midd_OSPort.h"
#include <poll.h>
#include <stdio.h>
#include "AppMessageHandlerManager.h"
OsQueue testQueue = OS_INVALID_QUEUE;
OsQueue testQueue2 = OS_INVALID_QUEUE;

static void keyboardInput(void *arg)
{


    printf("\n\n ------------ Keboard Input is Ready !!! \n\n\n");
    char in;


    struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI };
    char buff[256];

    while (1)
    {

        if( poll(&mypoll, 1, 1000) )
        {
            scanf("%c", &in);
        }

//        if ((testQueue != OS_INVALID_QUEUE) && (0 < zosMsgQueueReceive(testQueue, buff, 2048, WAIT_10_MS)))
//        {
//            printf("Viko Reply: %s \n\r", buff);
//        }
//
//        if ((testQueue != OS_INVALID_QUEUE) && (0 < zosMsgQueueReceive(testQueue2, buff, 2048, WAIT_10_MS)))
//        {
//            printf("GBox Reply: %s \n\r", buff);
//        }

        switch (in)
        {
            case 'a':
            case 'A':
            {
                printf("Pressed Ac Input\n");
                hwGpioExtInt(INPUT_AC_PIN);
                break;
            }
            case 's':
            case 'S':
            {
                printf("Pressed SuperCab\n");
                hwGpioExtInt(INPUT_SCAP_PIN);
                break;
            }
            case 'c':
            case 'C':
            {
                int s = appGsmMngIsNetworkReady();
                s = !s; //invert current status
                printf("Pressed Connection Button, Conn stat will be set to %s\n", s ? "ENABLE" : "DISABLE");
                appGsmMngSetConnStat(s);
                break;
            }
            case 'm':
            case 'M':
            {
                if (OS_INVALID_QUEUE == testQueue) {
                    testQueue = appMsgHandlerAddClient("TESTV");
                    testQueue2 = appMsgHandlerAddClient("TESTG");
                }
                else
                {
                    Msg_Handler_Message msg, msg2;

                    static int c = 0;
                    static char buff[128] = "";
                    sprintf(buff, "#02000000000010262790050$ %d", c++);

                    msg.msgType = EN_MSG_TYPE_VIKO;
                    msg.data = buff;
                    msg.length = strlen(buff);
                    appMsgHandlerHandleMsg("TESTV", &msg);

                    msg2.msgType = EN_MSG_TYPE_GRIDBOX;
                    msg2.data = "#FC|ZADA|123456|123456789012345|ZMET|1.0.3.4|20241114135613|26|+905301234567|1.03|ZADA";
                    msg2.length = strlen("#FC|ZADA|123456|123456789012345|ZMET|1.0.3.4|20241114135613|26|+905301234567|1.03|ZADA");
                    appMsgHandlerHandleMsg("TESTG", &msg2);
                }
                break;
            }
            case 'd':
            {
                appMsgHandlerRemoveClient("TESTV");
                appMsgHandlerRemoveClient("GBOXV");

                testQueue = OS_INVALID_QUEUE;
                testQueue2 = OS_INVALID_QUEUE;
            }
        }

        in = '\0';
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
int main (void)
{
    if (SUCCESS == appZMGwInit())
    {
        appZMGwStart(); //run baby run
        pthread_t thread;

        //Create a new thread
        pthread_create(&thread, NULL, keyboardInput, NULL);
        while(1) sleep(1);
    }

    return 0;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
//  __disable_irq();
  while (1)
  {
  }
}

/******************************** End Of File *********************************/
