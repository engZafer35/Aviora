/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 26 Mar 2024 - 17:21:30
* #File Name    : AppTimeService.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APPLICATION_APPZMETERGW_INC_APPTIMESERVICE_H__
#define __APPLICATION_APPZMETERGW_INC_APPTIMESERVICE_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include <time.h>
/******************************MACRO DEFINITIONS*******************************/

#define APP_TIME_SERVICE_NTP    (ENABLE) //Todo: ntp could be enabled or disabled with this macro.

/*******************************TYPE DEFINITIONS ******************************/
typedef struct
{
    S32 *ntpServer;
    U32 ntpPort;
}NtpServerConf;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appTimeServiceInit(const char *ntpServer, U32 ntpPort); //Todo: get configuration for ntp and other things

void appTimeServiceGetTime(struct tm *tm);

/*
 * When ntp is enabled, The time service doesn't need to be updated manually
 */
#if (APP_TIME_SERVICE_NTP == DISABLE)
RETURN_STATUS appTimeServiceSetTime(const TS_Time *tm);

RETURN_STATUS appTimeServiceSetTimerDate(S32 *timerID, const TS_Time *tm, VoidCallback cb);

RETURN_STATUS appTimeServiceStopTimerDate(S32 timerID);
#endif
#endif /* __APPLICATION_APPZMETERGW_INC_APPTIMESERVICE_H__ */

/********************************* End Of File ********************************/
