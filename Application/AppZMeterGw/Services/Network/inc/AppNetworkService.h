/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 2Mar 04, 2026 - 1:43:59 PM
* #File Name    : AppNetworkService.h
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
* 
******************************************************************************/
/****************************** IFNDEF & DEFINE ******************************/
#ifndef __APP_NETWORK_SERVICE_H__
#define __APP_NETWORK_SERVICE_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef struct {
    bool enabled;                   /**< GSM interface enabled flag */
    const char* devName;            /**< Device name (e.g., "quectelUC2000") */
    bool usePPP;                    /**< Use PPP protocol for connection */
    const char* connInterface;      /**< Connection interface (e.g., "huart1") */
    const char* srcPath;            /**< Resource path (e.g., "/g/uc2000_atP") */
} AppNetworkGsmConfig_t;

/**
 * Ethernet Interface Configuration
 * Generated from JSON Ethernet configuration block
 */
typedef struct {
    bool enabled;                   /**< Ethernet interface enabled flag */
    const char* devName;            /**< Device name (e.g., "ENC28J60") */
    const char* connInterface;      /**< Connection interface (e.g., "hspi2") */
    const char* srcPath;            /**< Resource path (e.g., "/e/ENC28J60") */
} AppNetworkEthConfig_t;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   Initialize and start all network interfaces
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS AppNetworkService_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_NETWORK_SERVICE_H__ */
