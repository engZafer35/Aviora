/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jun 8, 2024 - 6:12:05 PM
* #File Name    : uart_driver.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __UART_DRIVER_H__
#define __UART_DRIVER_H__
/*********************************INCLUDES*************************************/
//Dependencies
#include "core/net.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/


//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//UART driver
extern const UartDriver uartDriver;

//External interrupt related functions
error_t uartInit(void);
void uartEnableIrq(void);
void uartDisableIrq(void);
void uartStartTx(void);
void uartDrvSendDma(int line, void *buff, int leng);
//C++ guard
#ifdef __cplusplus
}
#endif

#endif

/********************************* End Of File ********************************/
