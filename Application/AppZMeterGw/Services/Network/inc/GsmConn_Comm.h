/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 11 Mar 2026 - 14:21:51
* #File Name    : GsmConn_Comm.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __GSM_CONN_COMM_H__
#define __GSM_CONN_COMM_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/
//Application configuration
//#define APP_PPP_PIN_CODE "0000"
#define APP_PPP_APN             "zd.iot"
#define APP_PPP_PHONE_NUMBER    "*99#"
#define APP_PPP_PRIMARY_DNS     "8.8.8.8"
#define APP_PPP_SECONDARY_DNS   "8.8.4.4"
#define APP_PPP_TIMEOUT         10000
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   Init Gsm Modem
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 **/
RETURN_STATUS GsmConnInit(void);

/**
 * @brief   Open a PPP session
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 **/
RETURN_STATUS GsmConnReconnect(void);

/**
 * @brief   Close a PPP session
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 **/
RETURN_STATUS GsmConnClosePPP(void);

/**
 * @brief   Check if the network is ready
 * @return  TRUE if the network is ready, FALSE otherwise
 **/
BOOL GsmConnIsNetworkReady(void);

#endif /* __GSM_CONN_COMM_H__ */

/********************************* End Of File ********************************/
