/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
* #Date         : 15 Mar 2026 - 12:46:00
* #File Name    : AppTimeService.h
*******************************************************************************/
/******************************************************************************
* This module implements an application time service that provides current time
* information to other application modules. It supports multiple time backends
* (e.g., NTP, internal RTC, external RTC, software tick) and can be configured
* to use any combination of these backends. The time service provides a unified
* API for getting the current time, converting between epoch and calendar time,
* and formatting time as strings. It also manages NTP synchronization and can
* update RTCs based on NTP time.
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_TIME_SERVICE_H__
#define __APP_TIME_SERVICE_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include <time.h>
/******************************MACRO DEFINITIONS*******************************/

#ifdef __cplusplus
extern "C" {
#endif
/*******************************TYPE DEFINITIONS ******************************/
typedef enum
{
    FIRST_YEAR = 0, /* yyyy.mm.dd hh:mm:ss */
    FIRST_DAY  = 1  /* dd.mm.yyyy hh:mm:ss */
} APP_TIME_STRING_FORMAT;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Initialize time service, including NTP sync if enabled
 * @param ntpHost NTP server hostname or IP address (null-terminated string)
 * @param ntpPort NTP server port
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceInit(const char *ntpHost, U16 ntpPort);

/**
 * @brief Set current time using epoch time
 * @param epoch Epoch time in seconds
 * @note if NTP is disabled, this will update RTCs and software tick backend (if used)
 *       if NTP is enabled, this will update RTCs and software tick backend (if used) and
 *       also override NTP sync until next successful NTP update
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceSetTime(U32 epoch);

/**
 * @brief Get current local time as struct tm
 * @param tmValue Output parameter for current local time (struct tm)
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceGetTime(struct tm *tmValue);

/**
 * @brief Get current epoch time
 * @return Current epoch time in seconds, or 0 if failed to get time
 */

U32 appTimeServiceGetEpoch(void);

/**
 * @brief Convert epoch time to struct tm
 * @param epoch Epoch time
 * @param outTm Output parameter for struct tm
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceEpochToTm(U32 epoch, struct tm *outTm);

/**
 * @brief Convert struct tm to epoch time
 * @param tmValue Input parameter for struct tm
 * @param outEpoch Output parameter for epoch time
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceTmToEpoch(const struct tm *tmValue, U32 *outEpoch);

/**
 * @brief Format current time as a string
 * @param buf Output buffer for formatted time string
 * @param bufSize Size of the output buffer
 * @param fmt Format specifier
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceFormatNow(char *buf, U32 bufSize, APP_TIME_STRING_FORMAT fmt);

/**
 * @brief Convert a time string to epoch time
 * @param str Input time string
 * @param fmt Format specifier
 * @param outEpoch Output parameter for epoch time
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceStringToEpoch(const char *str, APP_TIME_STRING_FORMAT fmt, U32 *outEpoch);

/**
 * @brief Convert a time string to struct tm
 * @param str Input time string
 * @param fmt Format specifier
 * @param outTm Output parameter for struct tm
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceStringToTm(const char *str, APP_TIME_STRING_FORMAT fmt, struct tm *outTm);

/**
 * @brief Set NTP server settings at runtime
 * @param host NTP server hostname or IP address (null-terminated string)
 * @param port NTP server port
 * @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
 */
RETURN_STATUS appTimeServiceSetNtpServer(const char *host, U16 port);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TIME_SERVICE_H__ */

/********************************* End Of File ********************************/


