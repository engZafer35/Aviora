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

                break;
            }
            case 'm':
            case 'M':
            {

                break;
            }
            case 'd':
            {

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
