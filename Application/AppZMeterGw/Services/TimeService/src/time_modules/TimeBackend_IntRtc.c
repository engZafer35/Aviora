/**
 * @file TimeBackend_IntRtc.c
 * @brief Internal RTC backend wrapper (middRTC)
 */
#define SHOW_PAGE_DBG_MSG  (DISABLE)

#include "Project_Conf.h"
#include "TimeService_RtcIf.h"

RETURN_STATUS appTimeIntRtcInit(void)
{
    /* middRTC layer init; stub if no init required */
    return SUCCESS;
}

RETURN_STATUS appTimeIntRtcGet(M4T11_RTC_STR *t)
{
    return middRtcIntGetTime(t);
}

RETURN_STATUS appTimeIntRtcSet(const M4T11_RTC_STR *t)
{
    return middRtcIntSetTime(t);
}

