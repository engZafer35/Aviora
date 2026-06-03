/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : March 19, 2026 - 13:36:00
* #File Name    : TimeBackend_SoftTick.c
*******************************************************************************/
/******************************************************************************
* This module implements the software tick time backend. 
* It uses a 1 second timer to increment the epoch time.
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "TimeBackend_SoftTick.h"

#include "MiddEventTimer.h"
#include "ZDebug.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
static volatile U32 gs_epochUtc = 0;
static S32 gs_timerId;
static volatile BOOL gs_inited = FALSE;

/***************************** STATIC FUNCTIONS  ******************************/
static void softTickCb(void)
{
    gs_epochUtc++;
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appTimeSoftTickInit(void)
{
    if (gs_inited)
    {
        return RETURN_SUCCESS;
    }

    gs_timerId = -1;
    if (RETURN_SUCCESS != middEventTimerRegister(&gs_timerId, softTickCb, WAIT_1_SEC, TRUE))
    {
        DEBUG_ERROR("->[E] softTick register failed");
        return RETURN_FAILURE;
    }

    if (RETURN_SUCCESS != middEventTimerStart(gs_timerId))
    {
        DEBUG_ERROR("->[E] softTick start failed");
        return RETURN_FAILURE;
    }

    gs_inited = TRUE;
    return RETURN_SUCCESS;
}

U32 appTimeSoftTickGetEpoch(void)
{
    return gs_epochUtc;
}

RETURN_STATUS appTimeSoftTickSetEpoch(U32 epochUtc)
{
    gs_epochUtc = epochUtc;
    return RETURN_SUCCESS;
}
/******************************** End Of File *********************************/
