/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 22 Mar 2024 - 13:25:43
* #File Name    : AppDeviceManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_DEVICE_MANAGER_H__
#define __APP_DEVICE_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef enum HARDWARE_UNITS_
{
    EN_HW_UART_DEBUG,
    EN_HW_ETH,
    EN_HW_UART_COMM_485,
    EN_HW_GSM,
    EN_HW_USB,

    EN_HW_UNIT_MAX_NUM
}HARDWARE_UNITS;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   Feed wdt
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appFeedWdt(void);

/**
 * @brief   Restart application and board, hard reset
 */
void appDevMngHwRestart(void);

/**
 * @brief   Restart application, soft reset
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
void appDevMngSwRestart(void);

/**
 * @brief   init all hardware units
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return EN_FAILURE
 */
RETURN_STATUS appDevMngInitHwUnits(void);

/**
 * @brief   close a hardware unit.
 * @param   hardware unit id
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appDevMngCloseHw(HARDWARE_UNITS unit);

/**
 * @brief   start a hardware unit
 * @param   hardware unit
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appDevMngStartHw(HARDWARE_UNITS unit);

/**
 * @brief   restart a hardware unit
 * @param   hardware unit
 * @return  if everything is OK, return EN_SUCCES
 *          otherwise return FAILURE
 */
RETURN_STATUS appDevMngRestartHw(HARDWARE_UNITS unit);

#endif /* __APP_DEVICE_MANAGER_H__ */

/********************************* End Of File ********************************/
