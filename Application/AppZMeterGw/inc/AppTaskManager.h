/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Apr 17, 2024 - 5:41:56 PM
* #File Name    : AppTaskManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_TASK_MANAGER_H__
#define __APP_TASK_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "Midd_OSPort.h"
/******************************MACRO DEFINITIONS*******************************/
#define MANAGE_MAX_TASK_NUMBER  (10) /** number of threads which will be tracked by this module */
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appTskMngInit(void);

/**
 * @brief Start task management, call this function after all other task is ready
 *
 */
void appTskMngStart(void);

OsTaskId appTskMngCreate(const char *name, OsTaskCode taskCode, void *arg, const ZOsTaskParameters *params, BOOL restart);

RETURN_STATUS appTskMngDelete(OsTaskId tid);

RETURN_STATUS appTskMngSuspend(OsTaskId tid);

RETURN_STATUS appTskMngSuspendAll(void);

RETURN_STATUS appTskMngResume(OsTaskId tid);

RETURN_STATUS appTskMngImOK(OsTaskId tid);

#endif /* __APP_TASK_MANAGER_H__ */

/********************************* End Of File ********************************/
