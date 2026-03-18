/**
 * @file TimeBackend_SoftTick.c
 * @brief Software time backend (epoch in RAM + 1s timer tick)
 */
#define SHOW_PAGE_DBG_MSG  (DISABLE)

#include "Project_Conf.h"

#include "MiddEventTimer.h"
#include "ZDebug.h"

static volatile U32 gs_epochUtc = 0;
static S32 gs_timerId = -1;
static volatile BOOL gs_inited = FALSE;

static void softTickCb(void)
{
    gs_epochUtc++;
}

RETURN_STATUS appTimeSoftTickInit(void)
{
    if (gs_inited)
    {
        return SUCCESS;
    }

    gs_timerId = -1;
    if (SUCCESS != middEventTimerRegister(&gs_timerId, softTickCb, WAIT_1_SEC, TRUE))
    {
        DEBUG_ERROR("->[E] TimeSrv: softTick register failed");
        return FAILURE;
    }

    if (SUCCESS != middEventTimerStart(gs_timerId))
    {
        DEBUG_ERROR("->[E] TimeSrv: softTick start failed");
        return FAILURE;
    }

    gs_inited = TRUE;
    return SUCCESS;
}

U32 appTimeSoftTickGetEpochUtc(void)
{
    return gs_epochUtc;
}

RETURN_STATUS appTimeSoftTickSetEpochUtc(U32 epochUtc)
{
    gs_epochUtc = epochUtc;
    return SUCCESS;
}

