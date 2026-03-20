/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : March 19, 2026 - 13:36:00
* #File Name    : TimeBackend_SoftTick.h
*******************************************************************************/
/******************************************************************************
* This module implements the software tick time backend. 
* It uses a 1 second timer to increment the epoch time.
*
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __TIME_BACKEND_SOFTTICK_H__
#define __TIME_BACKEND_SOFTTICK_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Initialize software tick time backend (epoch in RAM + 1s timer tick)
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS appTimeSoftTickInit(void);

/**
 * @brief Get current epoch time from software tick backend
 * @return Current epoch time in seconds, or 0 on failure
 */
U32 appTimeSoftTickGetEpochUtc(void);

/**
 * @brief Set current epoch time to software tick backend
 * @param epochUtc Epoch time in seconds to set
 * @return SUCCESS if successful, FAILURE otherwise
 */
RETURN_STATUS appTimeSoftTickSetEpochUtc(U32 epochUtc);

#endif /* __TIME_BACKEND_SOFTTICK_H__ */

/********************************* End Of File ********************************/
