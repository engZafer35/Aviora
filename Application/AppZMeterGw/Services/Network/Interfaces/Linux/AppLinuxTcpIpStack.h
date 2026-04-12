/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Cry For The Moon
* #Revision     : 1.0
* #Date         : 13 Apr 2026 - 00:223:02
* #File Name    : AppLinuxTcpIpStack.h
*******************************************************************************/
/******************************************************************************
* * This file contains the functions to manage the Linux TCP/IP stack.
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_LINUX_TCP_IP_STACK_H__
#define __APP_LINUX_TCP_IP_STACK_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief   Initialize the Linux TCP/IP stack
 * @return  if everything is OK, return SUCCESS
 *          otherwise return FAILURE
 */
RETURN_STATUS appLinuxTcpIpStackInit(void);


#endif /* __APP_LINUX_TCP_IP_STACK_H__ */

/********************************* End Of File ********************************/
