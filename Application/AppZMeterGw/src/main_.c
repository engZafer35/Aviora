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

static void keyboardInput(void *arg)
{
    printf("\n\n ------------ Keboard Input is Ready !!! \n\n\n");
    char in;

    while (1)
    {
        scanf("%c", &in);

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
        }
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
