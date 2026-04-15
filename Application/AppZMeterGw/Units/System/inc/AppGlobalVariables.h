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
//sw unit flags
#define FILE_SYSTEM_READY_FLAG        (0x0001)  // set when file system is ready, it is used by other sw units which have dependency on file system
#define DBUS_READY_FLAG               (0x0002)  // set when dbus is ready, it is used by other sw units which have dependency on dbus
#define TASK_MANAGER_READY_FLAG       (0x0004)  // set when task manager is ready, it is used by other sw units which have dependency on task manager
// service ready flags
#define LOGGER_SERVICE_READY_FLAG     (0x0008)  // set when logger service is ready, it is used by other sw units which have dependency on logger service
#define NETWORK_SERVICE_READY_FLAG    (0x0010)  // set when network service is ready, it is used by other sw units which have dependency on network service
#define TIME_SERVICE_READY_FLAG       (0x0020)  // set when time service is ready, it is used by other sw units which have dependency on time service
#define DISPLAY_SERVICE_READY_FLAG    (0x0040)  // set when display service is ready, it is used by other sw units which have dependency on display service
#define PROTOCOL_SERVICE_READY_FLAG   (0x0080)  // set when protocol service is ready, it is used by other sw units which have dependency on protocol service


//#define TIME_START_DEPENDENCY_FLAGS      0//(NETWORK_SERVICE_READY_FLAG) //if ntp is used wait for network service ready
#define LOGGER_WAIT_DEPENDENCY_FLAGS    (FILE_SYSTEM_READY_FLAG | DBUS_READY_FLAG | TASK_MANAGER_READY_FLAG | TIME_SERVICE_READY_FLAG)
//#define DISPLAY_START_DEPENDENCY_FLAGS   (LOGGER_SERVICE_READY_FLAG | TASK_MANAGER_READY_FLAG | DBUS_READY_FLAG)
//#define NETWORK_START_DEPENDENCY_FLAGS   (LOGGER_SERVICE_READY_FLAG | DBUS_READY_FLAG | TASK_MANAGER_READY_FLAG)
#define PROTOCOL_WAIT_DEPENDENCY_FLAGS  (LOGGER_SERVICE_READY_FLAG | FILE_SYSTEM_READY_FLAG | TASK_MANAGER_READY_FLAG | NETWORK_SERVICE_READY_FLAG | TIME_SERVICE_READY_FLAG)

/************************* GLOBAL VARIBALE REFERENCES *************************/
extern U32 g_protocol;
extern U32 g_meterHandlerList;
extern ZOsEventGroup gp_systemSetupEventGrp; 
/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appGlobalVarInit(void);

#endif /* _APP_GLOBAL_VARIABLES_H__ */

/********************************* End Of File ********************************/
