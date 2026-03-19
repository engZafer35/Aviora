/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : March 19, 2026 - 12:16:00
* #File Name    : TimeSync_Ntp.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __TIME_SYNC_NTP_H__
#define __TIME_SYNC_NTP_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/
#define NTP_HOST_NAME_MAX_LEN  (64)
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
* @brief Set NTP server settings at runtime
* @param host NTP server hostname or IP address (null-terminated string)
* @param port NTP server port
* @return RETURN_STATUS_SUCCESS if successful, RETURN_STATUS_FAILURE otherwise
*/
RETURN_STATUS appTimeNtpSetServer(const char *host, U16 port);

/**
 * @brief Query NTP server for current time (blocking call)
 * Note: This is a blocking call and may take several second
 * @return Return epoch time in secdonds, or 0 on failure
 */
U32 appTimeNtpGetEpochUtc(void);

#endif /* __TIME_SYNC_NTP_H__ */

/********************************* End Of File ********************************/
