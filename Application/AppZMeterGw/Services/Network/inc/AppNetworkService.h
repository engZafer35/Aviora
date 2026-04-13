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

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   Initialize and start all network interfaces
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appNetworkServiceStart(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_NETWORK_SERVICE_H__ */
