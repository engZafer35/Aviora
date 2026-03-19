/**
 * @file TimeService_Config.h
 * @brief AUTO-GENERATED configuration for TimeService
 */
#ifndef __CUS_TIME_SERVICE_CONFIG_H__
#define __CUS_TIME_SERVICE_CONFIG_H__

/* generated on: 2026-03-19 18:40:31 */

#define APP_TIME_SERVICE_USE              (1)
#define APP_TIME_SERVICE_USE_NTP          (0)
#define APP_TIME_SERVICE_USE_INT_RTC      (1)
#define APP_TIME_SERVICE_USE_EXT_RTC      (1)

/* timeZone: UTC => offset minutes */
#define APP_TIME_SERVICE_TZ_OFFSET_MINUTES (0)

#define APP_TIME_SERVICE_DEFAULT_NTP_HOST  "pool.ntp.org"
#define APP_TIME_SERVICE_DEFAULT_NTP_PORT  (123)
#define APP_TIME_SERVICE_NTP_UPDATE_PERIOD_MIN (60)

#include "../Driver/DeviceDrivers/inc/DrvM41T11_RTC.h"

#define EXT_RTC_I2C_ADDR  (0x68)
#define EXT_RTC_I2C_LINE  (I2C_LINE_1)

#include "../Driver/McuCoreDrivers/inc/McuInterruptRegister.h"

#endif /* __CUS_TIME_SERVICE_CONFIG_H__ */
