/**
 * @file TimeService_Config.h
 * @brief AUTO-GENERATED configuration for TimeService
 */
#ifndef __CUS_TIME_SERVICE_CONFIG_H__
#define __CUS_TIME_SERVICE_CONFIG_H__

/* generated on: 2026-03-20 00:15:48 */

#define APP_TIME_SERVICE_USE              (1)
#define APP_TIME_SERVICE_USE_NTP          (1)
#define APP_TIME_SERVICE_USE_INT_RTC      (1)
#define APP_TIME_SERVICE_USE_EXT_RTC      (0)

/* timeZone: UTC => offset minutes */
#define APP_TIME_SERVICE_TZ_OFFSET_MINUTES (0)

#define APP_TIME_SERVICE_DEFAULT_NTP_HOST  "pool.ntp.org"
#define APP_TIME_SERVICE_DEFAULT_NTP_PORT  (123)
#define APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN (60)

#define EXT_RTC_I2C_ADDR          (0)
#define EXT_RTC_I2C_LINE          (I2C_LINE_UNUSED)
#define EXT_RTC_INIT_FUNC(x)      (FAILURE)
#define EXT_RTC_GET_TIME_FUNC(t)  (FAILURE)
#define EXT_RTC_SET_TIME_FUNC(t)  (FAILURE)

#include "../Driver/McuCoreDrivers/inc/McuInterruptRegister.h"

#endif /* __CUS_TIME_SERVICE_CONFIG_H__ */
