/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jun 9, 2024 - 7:27:03 PM
* #File Name    : modem.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MODEM_H__
#define __MODEM_H__
/*********************************INCLUDES*************************************/

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
//Dependencies
#include "core/net.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Modem related functions
error_t modemInit(NetInterface *interface);
error_t modemConnect(NetInterface *interface);
error_t modemDisconnect(NetInterface *interface);

error_t modemSendAtCommand(NetInterface *interface,
   const char_t *command, char_t *response, size_t size);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif /* __MODEM_H__ */

/********************************* End Of File ********************************/
