/**
 * @file TimeBackend_ExtRtc.c
 * @brief External RTC backend wrapper (middRTC)
 */
#define SHOW_PAGE_DBG_MSG  (DISABLE)

#include "Project_Conf.h"
#include "TimeService_RtcIf.h"

RETURN_STATUS appTimeExtRtcInit(void)
{
    /* middRTC layer init; stub if no init required */
    return SUCCESS;
}

RETURN_STATUS appTimeExtRtcGet(M4T11_RTC_STR *t)
{
    return middRtcExtGetTime(t);
}

RETURN_STATUS appTimeExtRtcSet(const M4T11_RTC_STR *t)
{
    return middRtcExtSetTime(t);
}

