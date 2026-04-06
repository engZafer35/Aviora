/**
 * @author: Zafer Satilmis
 * @hype-man: Judas Priest with Tim Ripper Owens - Metal Gods * @file TimeService_Config.h
 * @brief AUTO-GENERATED configuration for TimeService
 */
#ifndef __CUS_TIME_SERVICE_CONFIG_H__
#define __CUS_TIME_SERVICE_CONFIG_H__

/* generated on: 2026-04-06 19:28:38 */

#define APP_TIME_SERVICE_USE              (1)
#define APP_TIME_SERVICE_USE_NTP          (1)
#define APP_TIME_SERVICE_USE_INT_RTC      (0)
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

#define INT_RTC_INIT_FUNC(x)      (FAILURE)
#define INT_RTC_GET_TIME_FUNC(t)  (FAILURE)
#define INT_RTC_SET_TIME_FUNC(t)  (FAILURE)

#endif /* __CUS_TIME_SERVICE_CONFIG_H__ */
