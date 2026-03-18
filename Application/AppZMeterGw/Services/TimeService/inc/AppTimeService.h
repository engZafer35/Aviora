/**
 * @file AppTimeService.h
 * @brief Application Time Service public API
 */
#ifndef __APP_TIME_SERVICE_H__
#define __APP_TIME_SERVICE_H__

#include "Project_Conf.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FIRST_YEAR = 0, /* yyyy.mm.dd hh:mm:ss */
    FIRST_DAY  = 1  /* dd.mm.yyyy hh:mm:ss */
} AppTimeStringFormat;

/**
 * @brief Initialize time service
 * @param ntpHost Optional runtime NTP host override (may be NULL)
 * @param ntpPort Optional runtime NTP port override (0 => use default)
 */
RETURN_STATUS appTimeServiceInit(const char *ntpHost, U16 ntpPort);

/**
 * @brief Get current local time as struct tm (legacy API)
 * @note This function is used by existing modules (e.g. logger)
 */
void appTimeServiceGetTime(struct tm *tmValue);

RETURN_STATUS appTimeServiceGetTm(struct tm *outTm);
RETURN_STATUS appTimeServiceGetEpoch(U32 *outEpoch);

RETURN_STATUS appTimeServiceEpochToTm(U32 epoch, struct tm *outTm);
RETURN_STATUS appTimeServiceTmToEpoch(const struct tm *tmValue, U32 *outEpoch);

RETURN_STATUS appTimeServiceFormatNow(char *buf, U32 bufSize, AppTimeStringFormat fmt);
RETURN_STATUS appTimeServiceStringToEpoch(const char *str, AppTimeStringFormat fmt, U32 *outEpoch);
RETURN_STATUS appTimeServiceStringToTm(const char *str, AppTimeStringFormat fmt, struct tm *outTm);

/**
 * @brief Update NTP server settings at runtime
 */
RETURN_STATUS appTimeServiceSetNtpServer(const char *host, U16 port);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TIME_SERVICE_H__ */

