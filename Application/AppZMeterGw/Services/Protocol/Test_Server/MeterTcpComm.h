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
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/
#define METER_TCP_SERVER_IP   "192.168.1.103"
#define METER_TCP_PORT        (5000)
/*******************************TYPE DEFINITIONS ******************************/
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

int meterTcpCommInit(void);

int meterTcpCommDeinit(void);

int meterTcpCommSend(const U8 *data, U32 dataLeng, U32 timeout);

int meterTcpCommReceive(U8 *data, U32 *dataLeng, U32 timeout);

int meterTcpCommSetBaudrate(U32 baudrate);


#endif /* __METER_TCP_COMM_H__ */

/********************************* End Of File ********************************/
