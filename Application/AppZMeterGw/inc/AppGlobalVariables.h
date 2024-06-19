/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 28 Mar 2024 - 21:17:41
* #File Name    : AppGlobalVariables.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef _APP_GLOBAL_VARIABLES_H__
#define _APP_GLOBAL_VARIABLES_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/
extern S32 g_sysLoggerID; /* After calling logger initialize with this variable, dont update it */

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appGlobalVarInit(void);

#endif /* _APP_GLOBAL_VARIABLES_H__ */

/********************************* End Of File ********************************/
