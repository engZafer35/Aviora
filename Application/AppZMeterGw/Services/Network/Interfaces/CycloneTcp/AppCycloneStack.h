/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Cry For The Moon
* #Revision     : 1.0
* #Date         : 13 Apr 2026 - 00:223:02
* #File Name    : AppCycloneStack.h
*******************************************************************************/
/******************************************************************************
* * This file contains the functions to manage the Cyclone TCP stack.
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_CYCLONE_STACK_H__
#define __APP_CYCLONE_STACK_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief   Initialize the Cyclone TCP/IP stack
 * @return  if everything is OK, return SUCCESS
 *          otherwise return FAILURE
 */
RETURN_STATUS appCycloneTcpIpStackInit(void);


#endif /* __APP_CYCLONE_STACK_H__ */

/********************************* End Of File ********************************/
