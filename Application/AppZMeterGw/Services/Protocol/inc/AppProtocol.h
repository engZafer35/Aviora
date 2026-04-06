/******************************************************************************
* #Author       : Zafer Satılmış
* #hype-man     : Pentagram(Mezarkabul) - Secret Missile
* #Revision     : 1.0
* #Date         : April 03, 2026
* #File Name    : AppProtocol.h
*******************************************************************************/

/******************************************************************************
*
*
******************************************************************************/
/****************************** IFNDEF & DEFINE ******************************/
#ifndef __APP_PROTOCOL_H__
#define __APP_PROTOCOL_H__
/********************************* INCLUDES **********************************/
#include "../../../../Customers/Protocol_Config.h"

/***************************** MACRO DEFINITIONS *****************************/
/**
 * @brief This macro initializes the active sensors and sets the error flag.
 *        If all sensors are initialized successfully, the error flag will be set to 0,
 *        otherwise it will be set to -1.
 * @param setErrorFlag This macro must be invoked before APP_INIT_PROTOCOLS called.
 */
#define APP_INIT_SENSORS(setErrorFlag)      INIT_SENSORS(setErrorFlag)

/**
 *
 * @param setErrorFlag This macro initializes the avtive protocols and sets the error flag.
 *                     If all protocols are initialized successfully, the error flag will be set to 0,
 *                     otherwise it will be set to -1.
 * @note This macro should be invoked after APP_INIT_SENSORS called, because protocols is depend on sensors.
 */
#define APP_INIT_PROTOCOLS(setErrorFlag, serialNum)    INIT_PROTOCOLS(setErrorFlag, serialNum)

/**
 * @brief This macro defines the number of active protocols.
 */
#define APP_ACTIVE_PROTOCOL_NUMBER  ACTIVE_PROTOCOL_NUMBER

/**
 * @brief This macro defines the number of active sensors.
 */
#define APP_ACTIVE_SENSOR_NUMBER    ACTIVE_SENSOR_NUMBER

/******************************* TYPE DEFINITIONS *****************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

#endif /* __APP_PROTOCOL_H__ */

/********************************* End Of File ********************************/
