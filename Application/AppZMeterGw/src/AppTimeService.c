/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 26 Mar 2024 - 17:21:38
* #File Name    : AppTimeService.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppTimeService.h"
#include "AppGlobalVariables.h"
#include "AppLogRecorder.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
#if (APP_TIME_SERVICE_NTP == ENABLE)
static RETURN_STATUS initNTP(void)
{
    return SUCCESS;
}

static RETURN_STATUS getTimeFromNTP(U32 *epochTime)
{
    return SUCCESS;
}

static RETURN_STATUS initRTC(void)
{
    return SUCCESS;
}

static RETURN_STATUS setRTCTime(const TS_Time *time)
{
    return SUCCESS;
}

static RETURN_STATUS getRTCTime(TS_Time *time)
{
    return SUCCESS;
}

#endif


/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appTimeServiceInit(NtpServerConf *ntp)
{
    RETURN_STATUS retVal;
    //TODO: init rtc, set sntp conf.

    /**
     * if ntp could not be initialized, we can just report it.
     * Don't need to restart the system. We can report this situation to server at least.
     * Also Hw rtc timer can be used until ntp problem solved.
     */
    if (FAILURE == initNTP())
    {
        DEBUG_ERROR("->[E] NTP init ERROR");
        appLogRec(g_sysLoggerID, "Error: NTP could not be initialized");
        //TODO: send this error over data bus
    }

    retVal = initRTC();
    if (FAILURE == retVal)
    {
        DEBUG_ERROR("->[E] RTC init ERROR");
        appLogRec(g_sysLoggerID, "Error: RTC could not be initialized");
        //TODO: send this error over data bus
    }

    return SUCCESS;
}

RETURN_STATUS appTimeServiceGetTime(TS_Time *tm)
{
    return SUCCESS;
}

RETURN_STATUS appTimeServiceSetTime(const TS_Time *tm)
{
    return SUCCESS;
}

RETURN_STATUS appTimeServiceSetTimerDate(S32 *timerID, const TS_Time *tm, VoidCallback cb)
{
    return SUCCESS;
}

RETURN_STATUS appTimeServiceStopTimerDate(S32 timerID)
{
    return SUCCESS;
}

/******************************** End Of File *********************************/
