/******************************************************************************
* #Author       : Auto-generated
* #Date         : 18 Mar 2026 - 19:13:24
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Generated from customer time_config.json. Only used units are included.
*******************************************************************************/

#include "Project_Conf.h"
#include "AppTimeService_Config.h"
#include "TimeService_RtcIf.h"

#include <string.h>

#include "time_modules/TimeSync_Ntp.c"

#include "time_modules/TimeBackend_IntRtc.c"

#include "time_modules/TimeBackend_ExtRtc.c"

static char g_ntpHost[128];
static U16 g_ntpPort;

RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
{
    if (IS_SAFELY_PTR(ntpHost) && ntpHost[0] != '\0')
    {
        strncpy(g_ntpHost, ntpHost, sizeof(g_ntpHost) - 1);
        g_ntpHost[sizeof(g_ntpHost) - 1] = '\0';
        g_ntpPort = (ntpPort == 0) ? (U16)APP_TIME_SERVICE_DEFAULT_NTP_PORT : ntpPort;
    }
    else
    {
        strncpy(g_ntpHost, APP_TIME_SERVICE_DEFAULT_NTP_HOST, sizeof(g_ntpHost) - 1);
        g_ntpHost[sizeof(g_ntpHost) - 1] = '\0';
        g_ntpPort = (U16)APP_TIME_SERVICE_DEFAULT_NTP_PORT;
    }
    (void)appTimeNtpSetServer(g_ntpHost, g_ntpPort);

    if (SUCCESS != appTimeIntRtcInit())
    {
        return FAILURE;
    }

    if (SUCCESS != appTimeExtRtcInit())
    {
        return FAILURE;
    }

    return SUCCESS;
}

RETURN_STATUS getEpochUtcFromPreferredSource(U32 *outEpochUtc)
{
    if (IS_NULL_PTR(outEpochUtc))
    {
        return FAILURE;
    }
    M4T11_RTC_STR r;
    if (SUCCESS != appTimeIntRtcGet(&r))
    {
        return FAILURE;
    }
    return appTimeServiceRtcStrToEpochUtc(&r, outEpochUtc);
}

void updateRtcsFromEpochUtc(U32 epochUtc)
{
    M4T11_RTC_STR r;

    (void)appTimeServiceEpochUtcToRtcStr(epochUtc, &r);

    (void)appTimeIntRtcSet(&r);
    (void)appTimeExtRtcSet(&r);
}

S32 appTimeServiceAutogenGetNtpEpochUtc(void)
{
    U32 e;
    if (SUCCESS == appTimeNtpGetEpochUtc(&e))
    {
        return (S32)e;
    }
    return -1;
}

RETURN_STATUS appTimeServiceAutogenSetNtpServer(const char *host, U16 port)
{
    if (IS_NULL_PTR(host) || host[0] == '\0')
    {
        return FAILURE;
    }
    strncpy(g_ntpHost, host, sizeof(g_ntpHost) - 1);
    g_ntpHost[sizeof(g_ntpHost) - 1] = '\0';
    g_ntpPort = (port == 0) ? (U16)APP_TIME_SERVICE_DEFAULT_NTP_PORT : port;
    return appTimeNtpSetServer(g_ntpHost, g_ntpPort);
}
