/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 22 Mar 2024 - 11:48:48
* #File Name    : App_ZMeterGw.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_ZMETERGW__
#define __APP_ZMETERGW__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * \brief   init Z_Meter_Gw application
 * \return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appZMGwInit(void);

/**
 * @brief   start Z_Meter_Gw application
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appZMGwStart(void);

#endif /* __APP_ZMETERGW__ */

/********************************* End Of File ********************************/
