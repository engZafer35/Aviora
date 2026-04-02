/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : April 3, 2026 - 00:05:26 AM
* #File Name    : MeterTcpComm.h
*******************************************************************************/

/******************************************************************************
*
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __METER_TCP_COMM_H__
#define __METER_TCP_COMM_H__
/*********************************INCLUDES*************************************/
#include <stdbool.h>
/******************************MACRO DEFINITIONS*******************************/
#define METER_TCP_SERVER_IP   "127.0.0.1"
#define METER_TCP_PORT        (1883)
/*******************************TYPE DEFINITIONS ******************************/
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

int meterTcpCommInit(void);

int meterTcpCommDeinit(void);

int meterTcpCommSend(const uint8_t *data, uint32_t dataLeng, uint32_t timeout);

int meterTcpCommReceive(uint8_t *data, uint32_t *dataLeng, uint32_t timeout);

int meterTcpCommSetBaudrate(uint32_t baudrate);


#endif /* __METER_TCP_COMM_H__ */

/********************************* End Of File ********************************/
