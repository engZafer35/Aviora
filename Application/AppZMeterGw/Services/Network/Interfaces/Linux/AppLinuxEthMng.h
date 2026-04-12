/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 12 Apr 2026 - 14:21:51
* #File Name    : AppLinuxEthMng.h
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
* 
******************************************************************************/
/****************************** IFNDEF & DEFINE ******************************/
#ifndef __APP_LINUX_ETH_MNG_H__
#define __APP_LINUX_ETH_MNG_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/


/**
 * @brief   Initialize TCP/IP stack
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS AppLinuxEthMngTcpIpStackInit(void);


/**
 * @brief   Initialize Network Interfaces
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS AppLinuxEthMngStart(void);

/**
 * @brief   Close LINUX_ETH MNG
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS AppLinuxEthMngClose(void);

/**
 * @brief   Reconnect LINUX_ETH network manager
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS AppLinuxEthMngReconnect(void);

/**
 * @brief   Check if LINUX_ETH network manager is ready
 * @return  if network is ready, return TRUE
 *          otherwise return FALSE
 */
BOOL AppLinuxEthMngIsNetworkReady(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_LINUX_ETH_MNG_H__ */
