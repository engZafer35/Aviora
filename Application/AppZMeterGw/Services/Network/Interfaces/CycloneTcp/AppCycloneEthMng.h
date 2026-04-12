/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Cry For The Moon
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 17:29:12
* #File Name    : AppCycloneEthMng.h
*******************************************************************************/
/******************************************************************************
* * This file contains the functions to manage the Ethernet connection for 
* the Cyclone TCP stack. It is used to initialize, close, reconnect and check 
* if the network is ready.
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_CYCLONE_ETH_MNG_H__
#define __APP_CYCLONE_ETH_MNG_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief   Initialize and start the Ethernet connection
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS CycloneEthMngStart(void);

/**
 * @brief   Close the Ethernet connection
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS CycloneEthMngClose(void);

/**
 * @brief   Reconnect to the network
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS CycloneEthMngReconnect(void);

/**
 * @brief   Check if the network is ready
 * @return  TRUE if the network is ready, FALSE otherwise
 */
BOOL CycloneEthMngIsNetworkReady(void);
#endif /* __APP_CYCLONE_ETH_MNG_H__ */

/********************************* End Of File ********************************/
