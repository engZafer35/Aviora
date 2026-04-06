/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : March 19, 2026 - 10:46:00
* #File Name    : MiddRTC.h
*******************************************************************************/
/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MIDD_RTC_H__
#define __MIDD_RTC_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "../../Customers/TimeService_Config.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef struct _RtcStr_t
{
    U8 sec;     /* Seconds parameter, from 00 to 59 */
    U8 min;     /* Minutes parameter, from 00 to 59 */
    U8 hour;    /* Hours parameter, 24Hour mode, 00 to 23 */
    U8 wday;    /* Day in a week, from 1 to 7 */
    U8 mday;    /* Date in a month, 1 to 31 */
    U8 mon;     /* Month in a year, 1 to 12 */
    U16 year;   /* Year parameter, 2000 to 3000 */
}RtcStr_t;

typedef RtcStr_t MiddRtcStr_t;

/********************************** VARIABLES *********************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/* Internal RTC */
/**
 * @brief Initialize internal RTC (if enabled in config)
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcIntInit(void);

/**
 * @brief Get time from internal RTC (if enabled in config)
 * @param t Output time struct
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcIntGetTime(RtcStr_t *t);

/**
 * @brief Set time to internal RTC (if enabled in config)
 * @param t Input time struct
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcIntSetTime(const RtcStr_t *t);

/* External RTC */
/**
 * @brief Initialize external RTC (if enabled in config)
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcExtInit(void);

/**
 * @brief Get time from external RTC (if enabled in config)
 * @param t Output time struct
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcExtGetTime(RtcStr_t *t);

/**
 * @brief Set time to external RTC (if enabled in config)
 * @param t Input time struct
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS middRtcExtSetTime(const RtcStr_t *t);

#endif /* __MIDD_RTC_H__ */

/********************************* End Of File ********************************/

