/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 11:01:57
* #File Name    : AppDisplayManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_DISPLAY_MANAGER_H__
#define __APP_DISPLAY_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/




/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief   Initialize the display manager
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appDisplayInit(void);

/**
 * @brief   Start the display manager task
 * @return  void
 */
void appDisplayStart(void);

/**
 * @brief Stop the display manager task, after this call, 
 *        display manager will not update display until 
 *        appDisplayStart is called again.
 * @return  void
 */
void appDisplayStop(void);

#endif /* __APP_DISPLAY_MANAGER_H__ */

/********************************* End Of File ********************************/
