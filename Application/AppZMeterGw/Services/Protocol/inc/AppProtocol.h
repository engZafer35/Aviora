/******************************************************************************
* #Author       : Zafer Satılmış
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
#define APP_INIT_PROTOCOLS(setErrorFlag)    INIT_PROTOCOLS(setErrorFlag)
#define APP_INIT_SENSORS(setErrorFlag)      INIT_SENSORS(setErrorFlag)

#define APP_ACTIVE_PROTOCOL_NUMBER          ACTIVE_PROTOCOL_NUMBER
#define APP_ACTIVE_SENDOR_NUMBER            ACTIVE_SENDOR_NUMBER

/******************************* TYPE DEFINITIONS *****************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

#endif /* __APP_PROTOCOL_H__ */

/********************************* End Of File ********************************/
