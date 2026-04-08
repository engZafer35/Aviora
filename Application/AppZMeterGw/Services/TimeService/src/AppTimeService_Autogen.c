/******************************************************************************
* #Author       : Auto-generated
* #Date         : 08 Apr 2026 - 23:37:12
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: LinuxGcc
* Generated from customer time_config.json. Only used units are included.
*******************************************************************************/

#include "Project_Conf.h"
#include "../../../Customers/TimeService_Config.h"

#include "../Middleware/MiddZModem/inc/MiddRTC.h"

static RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
{
    if (SUCCESS != middRtcExtInit())
    {
        return FAILURE;
    }

    return SUCCESS;
}

static U32 appTimeServiceAutogenGetEpochFromPreferredSource(void)
{
    MiddRtcStr_t r;
    U32 e = 0;
    if (SUCCESS == middRtcExtGetTime(&r))
    {
        //dont need to check return value here since 0 is an invalid epoch and indicates failure
        appTimeServiceRtcStrToEpoch(&r, &e);
    }
    return e;
}

static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpoch(U32 epoch)
{
    RETURN_STATUS retVal = SUCCESS;
    MiddRtcStr_t r;

    appTimeServiceEpochToRtcStr(epoch, &r);

    if (SUCCESS != middRtcExtSetTime(&r))
    {
        retVal = FAILURE;
    }
    return retVal;
}

static U32 appTimeServiceAutogenGetNtpEpoch(void)
{
    return 0;
}

static RETURN_STATUS appTimeServiceAutogenSetNtpServer(const char *host, U16 port)
{
    (void)host;
    (void)port;
    return FAILURE;
}
