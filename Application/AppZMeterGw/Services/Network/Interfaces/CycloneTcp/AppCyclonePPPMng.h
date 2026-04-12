/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Kingdom of Heaven
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 18:36:18
* #File Name    : AppCyclonePPPMng.h
*******************************************************************************/
/******************************************************************************
* * This file contains the functions to manage the PPP connection for 
* the Cyclone TCP stack. It is used to initialize, close, reconnect and check 
* if the network is ready.
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_CYCLONE_PPP_MNG_H__
#define __APP_CYCLONE_PPP_MNG_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief   Initialize and start the PPP connection
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS appCyclonePppMngStart(void);

/**
 * @brief   Close the PPP connection
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS appCyclonePppMngClose(void);

/**
 * @brief   Reconnect to the network
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS appCyclonePppMngReconnect(void);

/**
 * @brief   Check if the network is ready
 * @return  TRUE if the network is ready, FALSE otherwise
 */
BOOL appCyclonePppMngIsNetworkReady(void);
#endif /* __APP_CYCLONE_PPP_MNG_H__ */

/********************************* End Of File ********************************/
