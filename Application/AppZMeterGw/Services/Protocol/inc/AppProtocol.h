/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : April 02, 2026
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
#include "Project_Conf.h"
/***************************** MACRO DEFINITIONS *****************************/

#define TCP_DEFAULT_SERVER_IP   "127.0.0.1"
#define TCP_DEFAULT_PUSH_PORT   (1883)
#define TCP_DEFAULT_DEVICE_IP   "192.168.1.10"
#define TCP_DEFAULT_PULL_PORT   (2622)

#define APP_PROTOCOL_INIT_FUNC(serialNumber)   appProtocolInit(serialNumber)
#define APP_PROTOCOL_START_FUNC()              appProtocolStart()
#define APP_PROTOCOL_STOP_FUNC()               appProtocolStop()
#define APP_PROTOCOL_PUT_INCOMING_FUNC(channel, data, dataLength) appProtocolPutIncomingMessage(channel, data, dataLength)

#include "./Test_Server/MeterTcpComm.h"

#define SENSOR_INITIALIZE_FUNC()   \
                                    do{ \
                                        /* Sensor initialization code here */ \
                                        MeterCommInterface_t meterComm = { \
                                            .meterCommInit        = meterTcpCommInit, \
                                            .meterCommSend        = meterTcpCommSend, \
                                            .meterCommReceive     = meterTcpCommReceive, \
                                            .meterCommSetBaudrate = meterTcpCommSetBaudrate, \
                                        }; \
                                        appMeterOperationsStart(meterComm); \

                                    } while(0)

/******************************* TYPE DEFINITIONS *****************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

#endif /* __APP_PROTOCOL_H__ */

/********************************* End Of File ********************************/
