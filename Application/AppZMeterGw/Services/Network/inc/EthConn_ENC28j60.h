/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 11 Mar 2026 - 14:21:51
* #File Name    : EthConn_ENC28j60.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __ETH_CONN_ENC28J60_H__
#define __ETH_CONN_ENC28J60_H__
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
RETURN_STATUS EthConn_Enc28J60_Init(void);

/**
 * @brief   Close the Ethernet connection
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS EthConn_Enc28J60_CloseConn(void);

/**
 * @brief   Reconnect to the network
 * @return  if everything is OK, return EN_SUCCESS
 */
RETURN_STATUS EthConn_Enc28J60_Reconnect(void);

/**
 * @brief   Check if the network is ready
 * @return  TRUE if the network is ready, FALSE otherwise
 */
BOOL EthConn_Enc28J60IsNetworkReady(void);
#endif /* __ETH_CONN_ENC28J60_H__ */

/********************************* End Of File ********************************/
