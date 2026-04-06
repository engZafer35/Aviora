/******************************************************************************
* #Author       : Auto-generated
* #Date         : 06 Apr 2026 - 22:47:31
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: LinuxGcc
* Generated from customer time_config.json. Only used units are included.
*******************************************************************************/

#include "Project_Conf.h"
#include "../../../Customers/TimeService_Config.h"

#include "time_modules/TimeBackend_SoftTick.h"

static RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
{
    if (SUCCESS != appTimeSoftTickInit())
    {
        return FAILURE;
    }

    return SUCCESS;
}

static U32 appTimeServiceAutogenGetEpochUtcFromPreferredSource(void)
{
    return appTimeSoftTickGetEpochUtc();
}

static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpochUtc(U32 epochUtc)
{
    RETURN_STATUS retVal = SUCCESS;
    retVal = appTimeSoftTickSetEpochUtc(epochUtc);
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
