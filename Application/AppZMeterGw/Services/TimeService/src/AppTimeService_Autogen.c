/******************************************************************************
* #Author       : Auto-generated
* #Date         : 19 Mar 2026 - 18:40:31
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: ZD_0101
* Generated from customer time_config.json. Only used units are included.
*******************************************************************************/

#include "Project_Conf.h"
#include "../../../Customers/TimeService_Config.h"

#include "../Middleware/MiddZModem/inc/MiddRTC.h"

static RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
{
    if (SUCCESS != middRtcIntInit())
    {
        return FAILURE;
    }

    if (SUCCESS != middRtcExtInit())
    {
        return FAILURE;
    }

    return SUCCESS;
}

static U32 appTimeServiceAutogenGetEpochUtcFromPreferredSource(void)
{
    MiddRtcStr_t r;
    U32 e = 0;
    if (SUCCESS == middRtcIntGetTime(&r))
    {
        //dont need to check return value here since 0 is an invalid epoch and indicates failure
        appTimeServiceRtcStrToEpochUtc(&r, &e);
    }
    return e;
}

static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpochUtc(U32 epochUtc)
{
    RETURN_STATUS retVal = SUCCESS;
    MiddRtcStr_t r;

    appTimeServiceEpochUtcToRtcStr(epochUtc, &r);

    if (SUCCESS != middRtcIntSetTime(&r))
    {
        retVal = FAILURE;
    }
    if (SUCCESS != middRtcExtSetTime(&r))
    {
        retVal = FAILURE;
    }
    return retVal;
}

static U32 appTimeServiceAutogenGetNtpEpochUtc(void)
{
    return 0;
}

static RETURN_STATUS appTimeServiceAutogenSetNtpServer(const char *host, U16 port)
{
    (void)host;
    (void)port;
    return FAILURE;
}
