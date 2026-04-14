/******************************************************************************
* #Author       : Auto-generated
* #Date         : 13 Apr 2026 - 20:52:16
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: Stm407Eva
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

static U32 appTimeServiceAutogenGetEpochFromPreferredSource(void)
{
    return appTimeSoftTickGetEpoch();
}

static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpoch(U32 epoch)
{
    RETURN_STATUS retVal = SUCCESS;
    retVal = appTimeSoftTickSetEpoch(epoch);
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
