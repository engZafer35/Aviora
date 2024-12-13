/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Nov 22, 2024 - 6:54:01 PM
* #File Name    : AppMeterMessageHandler.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_METER_MESSAGE_HANDLER_H__
#define __APP_METER_MESSAGE_HANDLER_H__
/*********************************INCLUDES*************************************/
#include <AppMessageHandlerManager.h>
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/
#define METER_HANDLER_NAME "Z_ELECTRICITY_METER_HANDLER"
/*******************************TYPE DEFINITIONS ******************************/
typedef struct
{
    RETURN_STATUS (*meterSerialLineInit) (void);
    int           (*meterSerialLineSend)(U8 *data, U32 dataLeng, U32 timeout);
    int           (*meterSerialLineReceive)(U8 *data, U32 *dataLeng, U32 timeout);
    RETURN_STATUS (*meterSerialSetBaudrate)(U32 baudrate);
}MeterSerialInterface;

typedef enum MeterBrands
{
    METER_BRAND_LUNA,
    METER_BRAND_BAYLAN,
    METER_BRAND_KOHLER,
    METER_BRAND_MAKEL,
}MeterBrands_t;

typedef enum MeterTypes
{
    METER_TYPE_ELECTIRICTY,
    METER_TYPE_WATER,
    METER_TYPE_POWER,
    METER_TYPE_OTHER,
}MeterTypes_t;


typedef enum E_METER_COMMAND_LIST
{
    EN_E_METER_CMD_GET_ID,
    EN_E_METER_CMD_READ_OBIS,
    EN_E_METER_CMD_READOUT,
}E_METER_COMMAND_LIST_t;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appMeterMsgHandlerSetSerialInterface(MeterSerialInterface *meterInt);

RETURN_STATUS appMeterAddMeter(MeterTypes_t type, MeterBrands_t brand, int meterSerialNum);

RETURN_STATUS appMeterMsgHandler(Msg_Handler_Message* message, U8 *replyMsg, U32 *replyMsgLeng);



#endif /* __APP_METER_MESSAGE_HANDLER_H__ */

/********************************* End Of File ********************************/
