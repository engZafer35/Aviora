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
/******************************IFNDEF & DEFINE********************************/

/*********************************INCLUDES*************************************/
#include "lfs.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Create a mutex object for file system operations
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 */
int fsHelperMutexCreate(void);

/**
 * @brief Delete a mutex object for file system operations
 */
void fsHelperMutexDelete(void);

/**
 * @brief mutex lock for file system operations
 * @param c Pointer to the lfs configuration structure
 * @note param c is not used in this function
 */
void fsHelperMutexLock(const struct lfs_config *c);

/**
 * @brief mutex unlock for file system operations
 * @param c Pointer to the lfs configuration structure
 * @note param c is not used in this function
 */
void fsHelperMutexUnlock(const struct lfs_config *c);

/********************************* End Of File ********************************/