/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 23 Mar 2024 - 23:07:51
* #File Name    : AppGsmManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_GSM_MANAGER_H__
#define __APP_GSM_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/
//Application configuration
//#define APP_PPP_PIN_CODE "0000"
#define APP_PPP_APN             "mgbs"
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
RETURN_STATUS appGsmMngInit(void);

/**
 * @brief   Open a PPP session
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 **/
RETURN_STATUS appGsmMngOpenPPP(void);

/**
 * @brief   Close a PPP session
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 **/
RETURN_STATUS appGsmMngClosePPP(void);

void appGsmResetModem(void);
void appGsmPowerDownModem(void);
void appGsmPowerUpModem(void);

RETURN_STATUS appGsmMngGetNetworkStatus(void);

#endif /* __APP_GSM_MANAGER_H__ */

/********************************* End Of File ********************************/
