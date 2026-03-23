/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 23 Mar 2026 - 14:02:21
* #File Name    : fs_littlefs_helper_func.h
*******************************************************************************/
/******************************************************************************
* This file contains the helper functions for the file system operations
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)

/********************************* INCLUDES ***********************************/
#include "fs_helper_func.h"
#include "../../Midd_OS/inc/Midd_OSPort.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
/** 
 * fs mutex is created before kernel starting. Because of that you have to use 
 * static mutex initialization. Especially if freeRtos or orther rtos is used 
 * be aware of that. 
 * OsMutex contains static mutex buffer and osCreateMutex can use this buffer. 
 * You dont need to do anything for static mutex, just make sure that 
 * (configSUPPORT_STATIC_ALLOCATION 1) set to 1
 */
static OsMutex g_fsMutex; 

/***************************** PUBLIC FUNCTIONS  ******************************/
int fsHelperMutexCreate(void)
{
    return osCreateMutex(&g_fsMutex);
}

void fsHelperMutexDelete(void)
{
    osDeleteMutex(&g_fsMutex);
}

void fsHelperMutexLock(const struct lfs_config *c)
{
    (void)c;
    osAcquireMutex(&g_fsMutex);
}

void fsHelperMutexUnlock(const struct lfs_config *c)
{
    (void)c;
    osReleaseMutex(&g_fsMutex);
}

/******************************** End Of File *********************************/
