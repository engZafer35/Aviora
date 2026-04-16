/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
* #Date         : 15 Mar 2026 - 12:46:00
* #File Name    : AppTimeService.c
*******************************************************************************/
/******************************************************************************
* This module implements an application time service that provides current time
* information to other application modules. It supports multiple time backends
* (e.g., NTP, internal RTC, external RTC, software tick) and can be configured
* to use any combination of these backends. The time service provides a unified
* API for getting the current time, converting between epoch and calendar time,
* and formatting time as strings. It also manages NTP synchronization and can
* update RTCs based on NTP time.
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppTimeService.h"
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"

#include "MiddRTC.h"
#include "MiddEventTimer.h"

#include <string.h>
#include <stdio.h>

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef struct
{
    S32 ntpTimerId;
    BOOL initialized;
} AppTimeServiceCtx;

/********************************** VARIABLES *********************************/
static AppTimeServiceCtx gs_ts;

/***************************** STATIC FUNCTIONS  ******************************/

/* Include JSON-generated autogen (defines init, getEpoch, updateRtcs, getNtp) */
#include "AppTimeService_Autogen.c"

/* RTC <-> epoch conversion (used by autogen; tz from config) */
//RETURN_STATUS appTimeServiceRtcStrToEpoch(const MiddRtcStr_t *r, U32 *outEpoch);
//RETURN_STATUS appTimeServiceEpochToRtcStr(U32 epoch, MiddRtcStr_t *r);

static S32 tzOffsetSeconds(void)
{
    return (S32)APP_TIME_SERVICE_TZ_OFFSET_MINUTES * 60;
}

/* ---------- Minimal epoch<->calendar conversion helpers (UTC) ---------- */
static BOOL isLeapYear(int year)
{
    /* Gregorian rules */
    if ((year % 400) == 0) return TRUE;
    if ((year % 100) == 0) return FALSE;
    return ((year % 4) == 0) ? TRUE : FALSE;
}

static int daysInMonth(int year, int mon1to12)
{
    static const int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = mdays[mon1to12 - 1];
    if (mon1to12 == 2 && isLeapYear(year))
    {
        d = 29;
    }
    return d;
}

static void epochToTm(U32 epoch, struct tm *outTm)
{
    /* Convert seconds since 1970-01-01 00:00:00 UTC to broken-down time */
    U32 t = epoch;
    U32 sec = t % 60; t /= 60;
    U32 min = t % 60; t /= 60;
    U32 hour = t % 24; t /= 24;
    U32 days = t; /* days since epoch */

    int year = 1970;
    while (1)
    {
        U32 diy = isLeapYear(year) ? 366 : 365;
        if (days >= diy)
        {
            days -= diy;
            year++;
        }
        else
        {
            break;
        }
    }

    int mon = 1;
    while (1)
    {
        int dim = daysInMonth(year, mon);
        if (days >= (U32)dim)
        {
            days -= (U32)dim;
            mon++;
        }
        else
        {
            break;
        }
    }

    memset(outTm, 0, sizeof(*outTm));
    outTm->tm_sec  = (int)sec;
    outTm->tm_min  = (int)min;
    outTm->tm_hour = (int)hour;
    outTm->tm_mday = (int)days + 1;
    outTm->tm_mon  = mon - 1;        /* 0..11 */
    outTm->tm_year = year - 1900;    /* years since 1900 */

    /* tm_wday (0=Sunday). 1970-01-01 was Thursday (4) */
    outTm->tm_wday = (int)((epoch / 86400U + 4U) % 7U);
}

static RETURN_STATUS tmToEpoch(const struct tm *tmValue, U32 *outEpoch)
{
    if (IS_NULL_PTR(tmValue) || IS_NULL_PTR(outEpoch))
    {
        return FAILURE;
    }

    int year = tmValue->tm_year + 1900;
    int mon  = tmValue->tm_mon + 1; /* 1..12 */
    int mday = tmValue->tm_mday;

    if (year < 1970 || mon < 1 || mon > 12 || mday < 1 || mday > 31)
    {
        return FAILURE;
    }

    U32 days = 0;
    for (int y = 1970; y < year; y++)
    {
        days += isLeapYear(y) ? 366U : 365U;
    }
    for (int m = 1; m < mon; m++)
    {
        days += (U32)daysInMonth(year, m);
    }
    days += (U32)(mday - 1);

    U32 secs = days * 86400U
             + (U32)tmValue->tm_hour * 3600U
             + (U32)tmValue->tm_min  * 60U
             + (U32)tmValue->tm_sec;

    *outEpoch = secs;
    return SUCCESS;
}

static void rtcStrFromTm(const struct tm *tmValue, MiddRtcStr_t *outRtc)
{
    memset(outRtc, 0, sizeof(*outRtc));
    outRtc->sec  = (U8)tmValue->tm_sec;
    outRtc->min  = (U8)tmValue->tm_min;
    outRtc->hour = (U8)tmValue->tm_hour;
    outRtc->mday = (U8)tmValue->tm_mday;
    outRtc->mon  = (U8)(tmValue->tm_mon + 1);
    outRtc->year = (U16)(tmValue->tm_year + 1900);
    /* Map 0..6 (Sun..Sat) to 1..7 */
    outRtc->wday = (U8)((tmValue->tm_wday == 0) ? 1 : (tmValue->tm_wday + 1));
}

static RETURN_STATUS tmFromRtcStr(const MiddRtcStr_t *rtc, struct tm *outTm)
{
    if (IS_NULL_PTR(rtc) || IS_NULL_PTR(outTm))
    {
        return FAILURE;
    }

    memset(outTm, 0, sizeof(*outTm));
    outTm->tm_sec  = rtc->sec;
    outTm->tm_min  = rtc->min;
    outTm->tm_hour = rtc->hour;
    outTm->tm_mday = rtc->mday;
    outTm->tm_mon  = (int)rtc->mon - 1;
    outTm->tm_year = (int)rtc->year - 1900;
    /* Map 1..7 (Mon..Sun per comment) is ambiguous; treat 1..7 as 1..7 and clamp */
    if (rtc->wday >= 1 && rtc->wday <= 7)
    {
        /* Convert to 0..6, best-effort */
        outTm->tm_wday = (rtc->wday == 7) ? 0 : rtc->wday;
    }
    return SUCCESS;
}

static RETURN_STATUS appTimeServiceRtcStrToEpoch(const MiddRtcStr_t *r, U32 *outEpoch)
{
    struct tm tmLocal;
    U32 epochLocal;

    if (IS_NULL_PTR(r) || IS_NULL_PTR(outEpoch))
    {
        return FAILURE;
    }
    if (SUCCESS != tmFromRtcStr(r, &tmLocal))
    {
        return FAILURE;
    }
    if (SUCCESS != tmToEpoch(&tmLocal, &epochLocal))
    {
        return FAILURE;
    }

    S32 utc = (S32)epochLocal - tzOffsetSeconds();
    if (utc < 0) utc = 0;

    *outEpoch = (U32)utc;
    return SUCCESS;
}

static void appTimeServiceEpochToRtcStr(U32 epoch, MiddRtcStr_t *r)
{
    struct tm tmLocal;

    S32 localEpoch = (S32)epoch + tzOffsetSeconds();
    if (localEpoch < 0) localEpoch = 0;

    epochToTm((U32)localEpoch, &tmLocal);
    rtcStrFromTm(&tmLocal, r);
}

static void ntpTimerCb(void)
{
    S32 epoch;

    if (!gs_ts.initialized)
    {
        return;
    }

    epoch = appTimeServiceAutogenGetNtpEpoch();
    if (epoch >= 0)
    {
        DEBUG_INFO("->[I] NTP sync OK, epoch=%u", (U32)epoch);
        APP_LOG_REC(g_sysLoggerID, " NTP sync OK");
        if (FAILURE == appTimeServiceAutogenUpdateRtcsFromEpoch((U32)epoch))
        {
            DEBUG_ERROR("->[E] TimeSrv: Failed to update RTCs from NTP epoch");
            APP_LOG_REC(g_sysLoggerID, "TimeSrv: Failed to update RTCs from NTP epoch");
        }
    }
    else
    {
        DEBUG_ERROR("->[E] TimeSrv: NTP sync failed");
        APP_LOG_REC(g_sysLoggerID, "TimeSrv: NTP sync failed");
    }
}

static RETURN_STATUS formatTm(const struct tm *t, char *buf, U32 bufSize, APP_TIME_STRING_FORMAT fmt)
{
    if (IS_NULL_PTR(t) || IS_NULL_PTR(buf) || bufSize < 20)
    {
        return FAILURE;
    }

    int year = t->tm_year + 1900;
    int mon  = t->tm_mon + 1;
    int day  = t->tm_mday;

    if (fmt == FIRST_YEAR)
    {
        (void)snprintf(buf, bufSize, "%04d.%02d.%02d %02d:%02d:%02d",
                       year, mon, day, t->tm_hour, t->tm_min, t->tm_sec);
    }
    else
    {
        (void)snprintf(buf, bufSize, "%02d.%02d.%04d %02d:%02d:%02d",
                       day, mon, year, t->tm_hour, t->tm_min, t->tm_sec);
    }
    buf[bufSize - 1] = '\0';
    return SUCCESS;
}

static RETURN_STATUS parseDateTime(const char *str, APP_TIME_STRING_FORMAT fmt, struct tm *outTm)
{
    if (IS_NULL_PTR(str) || IS_NULL_PTR(outTm))
    {
        return FAILURE;
    }

    int y, mo, d, hh, mm, ss;
    if (fmt == FIRST_YEAR)
    {
        if (6 != sscanf(str, "%d.%d.%d %d:%d:%d", &y, &mo, &d, &hh, &mm, &ss))
        {
            return FAILURE;
        }
    }
    else
    {
        if (6 != sscanf(str, "%d.%d.%d %d:%d:%d", &d, &mo, &y, &hh, &mm, &ss))
        {
            return FAILURE;
        }
    }

    memset(outTm, 0, sizeof(*outTm));
    outTm->tm_year = y - 1900;
    outTm->tm_mon  = mo - 1;
    outTm->tm_mday = d;
    outTm->tm_hour = hh;
    outTm->tm_min  = mm;
    outTm->tm_sec  = ss;
    return SUCCESS;
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appTimeServiceSetNtpServer(const char *host, U16 port)
{
    if (IS_NULL_PTR(host) || (0 == host[0]))
    {
        return FAILURE;
    }
    return appTimeServiceAutogenSetNtpServer(host, port);
}

RETURN_STATUS appTimeServiceInit(const char *ntpHost, U16 ntpPort)
{
    RETURN_STATUS retVal = SUCCESS;

    memset(&gs_ts, 0, sizeof(gs_ts));

    if (SUCCESS != appTimeServiceAutogenInit(ntpHost, ntpPort))
    {
        DEBUG_ERROR("->[E] autogen init failed");        
        return FAILURE;
    }

#if (APP_TIME_SERVICE_USE_NTP == 1)
    gs_ts.ntpTimerId = -1;
    {
        U32 periodMs = (U32)APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN * WAIT_1_MIN;
        if (SUCCESS != middEventTimerRegister(&gs_ts.ntpTimerId, ntpTimerCb, periodMs, TRUE))
        {
            DEBUG_ERROR("->[E] TimeSrv: NTP timer register failed");
            retVal = FAILURE;
        }
        else
        {
            (void)middEventTimerStart(gs_ts.ntpTimerId);
            ntpTimerCb();
        }
    }
#endif

    gs_ts.initialized = TRUE;
    return retVal;
}

RETURN_STATUS appTimeServiceSetTime(U32 epoch)
{
    RETURN_STATUS retVal = SUCCESS;
    
    if (!gs_ts.initialized)
    {
        return FAILURE;
    }

    if (FAILURE == appTimeServiceAutogenUpdateRtcsFromEpoch((U32)epoch))
    {
        DEBUG_ERROR("->[E] User time update Failed");
        APP_LOG_REC(g_sysLoggerID, "TimeSrv: User time update Failed");
        retVal = FAILURE;
    }
    else
    {
        DEBUG_INFO("->[I] User time update OK, epoch=%u", (U32)epoch);
        APP_LOG_REC(g_sysLoggerID, "TimeSrv: User time update OK");
    }

    return retVal;
}

RETURN_STATUS appTimeServiceGetTime(struct tm *tmValue)
{
    RETURN_STATUS retVal = FAILURE;
    U32 epoch;

    if (IS_NULL_PTR(tmValue))
    {
        return FAILURE;
    }

    epoch = appTimeServiceAutogenGetEpochFromPreferredSource();
//    if (epoch > 0)
    {
        retVal = appTimeServiceEpochToTm(epoch, tmValue);        
    }

    return retVal;
}

U32 appTimeServiceGetEpoch(void)
{
    return appTimeServiceAutogenGetEpochFromPreferredSource();
}

RETURN_STATUS appTimeServiceEpochToTm(U32 epoch, struct tm *outTm)
{
    /* Input epoch is UTC, output is local */
    S32 localEpoch = (S32)epoch + tzOffsetSeconds();
    if (localEpoch < 0) localEpoch = 0;

    epochToTm((U32)localEpoch, outTm);

    return SUCCESS;
}

RETURN_STATUS appTimeServiceTmToEpoch(const struct tm *tmValue, U32 *outEpoch)
{
    RETURN_STATUS retVal = FAILURE;
    U32 localEpoch;
    
    if (SUCCESS == tmToEpoch(tmValue, &localEpoch))
    {
        S32 utc = (S32)localEpoch - tzOffsetSeconds();
        if (utc < 0) utc = 0;
        *outEpoch = (U32)utc;
        retVal = SUCCESS;
    }

    return retVal;
}

RETURN_STATUS appTimeServiceFormatNow(char *buf, U32 bufSize, APP_TIME_STRING_FORMAT fmt)
{
    struct tm t;
    if (SUCCESS != appTimeServiceGetTime(&t))
    {
        return FAILURE;
    }
    return formatTm(&t, buf, bufSize, fmt);
}

RETURN_STATUS appTimeServiceStringToTm(const char *str, APP_TIME_STRING_FORMAT fmt, struct tm *outTm)
{
    return parseDateTime(str, fmt, outTm);
}

RETURN_STATUS appTimeServiceStringToEpoch(const char *str, APP_TIME_STRING_FORMAT fmt, U32 *outEpoch)
{
    RETURN_STATUS retVal = FAILURE;
    struct tm t;

    if (SUCCESS == parseDateTime(str, fmt, &t))
    {
        retVal = appTimeServiceTmToEpoch(&t, outEpoch);
    }

    return retVal;
}

