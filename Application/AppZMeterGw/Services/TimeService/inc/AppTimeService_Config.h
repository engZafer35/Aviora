/**
 * @file AppTimeService_Config.h
 * @brief AUTO-GENERATED configuration for TimeService
 */
#ifndef __APP_TIME_SERVICE_CONFIG_H__
#define __APP_TIME_SERVICE_CONFIG_H__

#include "Project_Conf.h"

/* generated on: 2026-03-19 02:14:39 */

#define APP_TIME_SERVICE_USE              (1)
#define APP_TIME_SERVICE_USE_NTP          (1)
#define APP_TIME_SERVICE_USE_INT_RTC      (1)
#define APP_TIME_SERVICE_USE_EXT_RTC      (0)

/* timeZone: UTC => offset minutes */
#define APP_TIME_SERVICE_TZ_OFFSET_MINUTES (0)

#define APP_TIME_SERVICE_DEFAULT_NTP_HOST  "pool.ntp.org"
#define APP_TIME_SERVICE_DEFAULT_NTP_PORT  (123)
#define APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN (60)

#endif /* __APP_TIME_SERVICE_CONFIG_H__ */
