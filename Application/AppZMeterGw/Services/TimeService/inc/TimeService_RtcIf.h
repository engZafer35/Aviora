/**
 * @file TimeService_RtcIf.h
 * @brief RTC interface for TimeService (midd layer)
 *
 * middRTC layer must not do epoch/timezone conversions.
 * It only reads/writes calendar time represented by M4T11_RTC_STR.
 */
#ifndef __TIME_SERVICE_RTC_IF_H__
#define __TIME_SERVICE_RTC_IF_H__

#include "Project_Conf.h"

typedef struct _M4T11_RTC_STR
{
    U8 sec;     /* Seconds parameter, from 00 to 59 */
    U8 min;     /* Minutes parameter, from 00 to 59 */
    U8 hour;    /* Hours parameter, 24Hour mode, 00 to 23 */
    U8 wday;    /* Day in a week, from 1 to 7 */
    U8 mday;    /* Date in a month, 1 to 31 */
    U8 mon;     /* Month in a year, 1 to 12 */
    U16 year;   /* Year parameter, 2000 to 3000 */
} M4T11_RTC_STR;

/* Internal RTC */
RETURN_STATUS middRtcIntGetTime(M4T11_RTC_STR *t);
RETURN_STATUS middRtcIntSetTime(const M4T11_RTC_STR *t);

/* External RTC */
RETURN_STATUS middRtcExtGetTime(M4T11_RTC_STR *t);
RETURN_STATUS middRtcExtSetTime(const M4T11_RTC_STR *t);

#endif /* __TIME_SERVICE_RTC_IF_H__ */

