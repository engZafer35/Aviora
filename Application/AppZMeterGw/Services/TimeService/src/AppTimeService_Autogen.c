/******************************************************************************
* #Author       : Auto-generated
* #Date         : 20 Apr 2026 - 08:11:29
* #File Name    : AppTimeService_Autogen.c
*******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: LinuxGcc
* Generated from customer time_config.json. Only used units are included.
*******************************************************************************/

#include "Project_Conf.h"
#include "../../../Customers/TimeService_Config.h"


static RETURN_STATUS appTimeServiceAutogenInit(const char *ntpHost, U16 ntpPort)
{
    // linux local time is used
    return SUCCESS;
}

static U32 appTimeServiceAutogenGetEpochFromPreferredSource(void)
{
    return getCurrentUnixTime();
}

static RETURN_STATUS appTimeServiceAutogenUpdateRtcsFromEpoch(U32 epoch)
{
    RETURN_STATUS retVal = SUCCESS;
    if (0 != linuxLocalTimeSvcSetEpoch(epoch))
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
