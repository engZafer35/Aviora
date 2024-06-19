/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : MiddEventTimer.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MIDD_EVENT_TIMER_H__
#define __MIDD_EVENT_TIMER_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/
#define EN_TIMER_MAX_NUM    (10)

#define MIN_TIMER_PERIOD_MS (10)
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS middEventTimerInit (void);

RETURN_STATUS middEventTimerRegister(S32 *timerId,  VoidCallback callback, U32 timeMs, BOOL isPeriodic);

RETURN_STATUS middEventTimerStart(S32 id);

/**
 * @brief  stop timer
 * @param  timer id
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 * @note if the timer is periodic timer, it can be restart without re-register
 *       just call time start function to start it.
 *       However if it is not periodic timer, after calling timer stop function
 *       it also will be deleted from timer list
 */
RETURN_STATUS middEventTimerStop(S32 id);

RETURN_STATUS middEventTimerStopAll(void);

RETURN_STATUS middEventTimerDelete(S32 id);

RETURN_STATUS middEventTimerDeleteAll(void);

#endif /* __MIDD_EVENT_TIMER_H__ */

/********************************* End Of File ********************************/
