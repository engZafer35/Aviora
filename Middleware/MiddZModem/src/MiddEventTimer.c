/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : MiddEventTimer.c 
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/*********************************INCLUDES*************************************/
#include "MiddEventTimer.h"

#ifdef OS_BAREMETAL
#include "McuInterruptRegister.h"
#endif
/****************************** MACRO DEFINITIONS *****************************/
#define IS_VALID_TIMER_ID(x)    (x < EN_TIMER_MAX_NUM)
/*************************** FUNCTION PROTOTYPES ******************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef struct _TimerStr
{
    U32 timeout;
    VoidCallback callback;
    U32 timeoutCopy;
    BOOL periodic;

}TimerStr;
/********************************** VARIABLES *********************************/
static volatile TimerStr timerCallBackList[EN_TIMER_MAX_NUM] = {{0},};

/***************************** STATIC FUNCTIONS  ******************************/

/*
 * Check user timer, İf the time reached timer call callback function
 * And than start again
 */
static void timerEventCallback(int x)
{
    static USS i;
return;
    for (i = 0; i < EN_TIMER_MAX_NUM; i++)
    {
        if (timerCallBackList[i].timeout > 0)
        {
            if (IS_SAFELY_PTR(timerCallBackList[i].callback))
            {
                timerCallBackList[i].timeout -= MIN_TIMER_PERIOD_MS;

                //timer finished. invoke callback func
                if (timerCallBackList[i].timeout < MIN_TIMER_PERIOD_MS)
                {
                    timerCallBackList[i].callback();

                    if (TRUE == timerCallBackList[i].periodic)
                    {
                        timerCallBackList[i].timeout = timerCallBackList[i].timeoutCopy; //restart timer
                    }
                    else
                    {
                        timerCallBackList[i].timeout     = 0;
                        timerCallBackList[i].timeoutCopy = 0;
                        timerCallBackList[i].callback    = NULL;
                        //timerCallBackList[i].periodic    = FALSE; //it is alread false
                    }
                }
            }
        }
    }
}

#ifdef WORKING_PLATFORM_PC
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
static void testThread(void)
{
    printf("\n#>middEvent TestThread Running ... \n");
    while(1)
    {
        usleep(10000);
        timerEventCallback(0);
    }
}
#endif
/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS middEventTimerInit(void)
{
    RETURN_STATUS retVal = SUCCESS;;
    USS i = 0;

    //clear callback list
    for (i = 0; i < EN_TIMER_MAX_NUM; i++)
    {
        timerCallBackList[i].timeout = 0;
        timerCallBackList[i].callback = NULL;
    }

#ifdef OS_BAREMETAL
    retVal = drvIntRegister(timerEventCallback, TIMER_EVENT_IT_ID);
    DBG_MSG("-> [I]middEventTimerInit return: %d", retVal);
#endif


#ifdef USE_FREERTOS
    //TODO: create 10 ms timer in freertos
#endif

#ifdef WORKING_PLATFORM_PC
    pthread_t thread_id;
    if (0 == pthread_create(&thread_id, NULL, (void *)testThread, NULL))
    {
        printf("----EVENT TIMER Test Thread Created ..... \n");
    }
#endif

    return retVal;
}

RETURN_STATUS middEventTimerRegister(S32 *timerId,  VoidCallback callback, U32 timeMs, BOOL isPeriodic)
{
    RETURN_STATUS retVal = FAILURE;
    U32 ru;
    U32 i;

    *timerId = -1; //firstly load invalid id

    if (IS_SAFELY_PTR(callback))
    {
        return retVal;
    }

    for(i = 0; i < EN_TIMER_MAX_NUM; i++)
    {
        if (NULL == timerCallBackList[i].callback) //find empty index
        {
            //round up
            ru = (timeMs % MIN_TIMER_PERIOD_MS);
            if (0 != ru)
            {
                timeMs += MIN_TIMER_PERIOD_MS - ru;
            }

            timerCallBackList[i].callback    = callback;
            timerCallBackList[i].timeout     = 0;
            timerCallBackList[i].timeoutCopy = timeMs;
            timerCallBackList[i].periodic    = isPeriodic;

            *timerId = i;
            retVal = SUCCESS;
        }
    }
    return retVal;
}

RETURN_STATUS middEventTimerStart(S32 id)
{
    RETURN_STATUS retVal = FAILURE;

    if (IS_VALID_TIMER_ID(id))
    {
        timerCallBackList[id].timeout = timerCallBackList[id].timeoutCopy;
        retVal = SUCCESS;
    }

    return retVal;
}

RETURN_STATUS middEventTimerStop(S32 id)
{
    int retVal = FAILURE;

    if (IS_VALID_TIMER_ID(id))
    {
        timerCallBackList[id].timeout = 0;
        if (FALSE == timerCallBackList[id].periodic) //if it is not periodic timer, in this case delete it
        {
            timerCallBackList[id].callback    = NULL;
            timerCallBackList[id].timeoutCopy = 0;
            timerCallBackList[id].periodic    = FALSE;
        }
        retVal = SUCCESS;
    }

    return retVal;
}

RETURN_STATUS middEventTimerStopAll(void)
{
    USS i;
    for(i = 0; i < EN_TIMER_MAX_NUM; i++)
    {
        timerCallBackList[i].timeout = 0;
        if (FALSE == timerCallBackList[i].periodic) //if it is not periodic timer, in this case delete it
        {
            timerCallBackList[i].callback    = NULL;
            timerCallBackList[i].timeoutCopy = 0;
            timerCallBackList[i].periodic    = FALSE;
        }
    }

    return SUCCESS;
}

RETURN_STATUS middEventTimerDelete(S32 id)
{
    RETURN_STATUS retVal = FAILURE;

    if (IS_VALID_TIMER_ID(id))
    {
        timerCallBackList[id].timeout     = 0;
        timerCallBackList[id].timeoutCopy = 0;
        timerCallBackList[id].callback    = NULL;
        timerCallBackList[id].periodic     = FALSE;

        retVal = SUCCESS;
    }

    return retVal;
}

RETURN_STATUS middEventTimerDeleteAll(void)
{
    USS i;
    for(i = 0; i < EN_TIMER_MAX_NUM; i++)
    {
        timerCallBackList[i].timeout     = 0;
        timerCallBackList[i].timeoutCopy = 0;
        timerCallBackList[i].callback    = NULL;
        timerCallBackList[i].periodic    = FALSE;
    }

    return SUCCESS;
}

/******************************** End Of File *********************************/
