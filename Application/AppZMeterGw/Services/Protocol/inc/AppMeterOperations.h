/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 2.0
* #Date         : Mar 27, 2026 - 20:23:22 PM
* #File Name    : AppMeterOperations.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_METER_OPERATIONS_H__
#define __APP_METER_OPERATIONS_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/
#define METER_MAIN_DIR         "./meter/"
/** Binary tablo: ardışık MeterTable kayıtları (boyut / sizeof(MeterTable) = sayaç adedi) */
#define METER_LIST_FILE        METER_MAIN_DIR "meterList"
/** Kayıtlı sayaç verisi: METER_REG_DIR + seri no (16 byte'a kadar) */
#define METER_REG_DIR          METER_MAIN_DIR "reg/"
#define METER_DATA_OUTPUT_DIR  METER_MAIN_DIR "meterDataOut/"
#define DIRECTIVE_MAIN_DIR     METER_MAIN_DIR "directive/"


/*******************************TYPE DEFINITIONS ******************************/

typedef enum ERR_CODE_T
{
    EN_ERR_CODE_SUCCESS = 0,
    EN_ERR_CODE_PENDING = 1,
    EN_ERR_CODE_FAILURE = -1,
    EN_ERR_CODE_TIMEOUT = -2,
    EN_ERR_CODE_METER_NOT_FOUND = -3,
    EN_ERR_CODE_METER_PARAM_ERROR = -4,
    EN_ERR_CODE_DIRECTIVE_NOT_FOUND = -5,
    EN_ERR_CODE_DIRECTIVE_PARAM_ERROR = -6,
    EN_ERR_CODE_METER_COMM_LINE_ERROR = -7,
    EN_ERR_CODE_METER_COMM_LINE_NOT_FOUND = -8,
    EN_ERR_CODE_METER_COMM_LINE_PARAM_ERROR = -9,
    EN_ERR_CODE_NOT_IMPLEMENTED = -10,
    EN_ERR_CODE_NO_RESOURCES = -11,

}ERR_CODE_T;

typedef void (*Callback_t)(S32 taskID, ERR_CODE_T status);

typedef struct
{
    RETURN_STATUS (*meterCommInit)(void);
    S32 (*meterCommSend)(const uint8_t *data, uint32_t dataLeng, uint32_t timeout);
    S32 (*meterCommReceive)(uint8_t *data, uint32_t *dataLeng, uint32_t timeout);
    RETURN_STATUS (*meterCommSetBaudrate)(uint32_t baudrate);
}MeterCommInterface_t;

typedef enum MeterBrands
{
    EN_METER_BRAND_BAYLAN,
    EN_METER_BRAND_KOHLER,
    EN_METER_BRAND_MAKEL,
}MeterBrands_t;

typedef enum MeterTypes
{
    EN_METER_TYPE_ELECTIRICTY,
    EN_METER_TYPE_WATER,
    EN_METER_TYPE_POWER,
    EN_METER_TYPE_OTHER,
}MeterTypes_t;

typedef enum E_METER_COMMAND_LIST
{
    EN_E_METER_CMD_GET_ID,
    EN_E_METER_CMD_READ_OBIS,
    EN_E_METER_CMD_READOUT,
}E_METER_COMMAND_LIST_t;

/*
"meters": {
    "protocol:": "IEC62056",
    "type": "electricity|water|gas|...",
    "brand": "MKL",
    "serialNumber": "12345678",
    "serialPort": "rs485-1",
    "initBaud": 300,
    "fixBaud": false,
    "frame": "7E1"
    }
    */
typedef struct
{
    char protocol[16];
    char type[16];
    char brand[4];
    char customerNumber[16];
    char serialNumber[16];
    char serialPort[8];
    int initBaud;
    BOOL fixBaud;
    char frame[8];
}MeterData_t;

typedef struct MeterTable
{
    char serialNumber[16];
} MeterTable_t;


/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief Start the meter operations task
 * @param meterComm Pointer to the meter communication interface
 * @return RETURN_STATUS SUCCESS if the meter operations is initialized successfully, FAILURE otherwise
 */
RETURN_STATUS appMeterOperationsStart(MeterCommInterface_t *meterComm);

/**
 * @brief Add a meter to the meter operations
 * @param meterData Pointer to the meter data
 * @return RETURN_STATUS SUCCESS if the meter is added successfully, FAILURE otherwise
 */
RETURN_STATUS appMeterOperationsAddMeter(MeterData_t *meterData);

RETURN_STATUS appMeterOperationsGetMeterData(const char *serialNumber, MeterData_t *meterDataOut);

RETURN_STATUS appMeterOperationsGetMeterDataByIndex(U32 index, MeterData_t *meterDataOut);

RETURN_STATUS appMeterOperationsDeleteMeter(const char *serialNumber);

RETURN_STATUS appMeterOperationsDeleteAllMeters(void);

U32 appMeterOperationsGetMeterCount(void);

//example requests
/*
"requestProfile": {
"directive" : "ProfileDirective1",
"METERSERIALNUMBER" : "12345678",
"startDate": "2021-06-22 00:00:00",
"endDate": "2021-06-22 12:05:00"
}

"requestReadout": {
"directive": "ReadoutDirective1",
"parameters": {
"METERSERIALNUMBER": "12345678"
}

"requestObis": {
"directive" : "WriteDirective1",
"password": "12345678",
"data": "0.9.2(21-07-28)"
}
*/

S32 appMeterOperationsAddReadoutTask(const char *request, Callback_t callback);

S32 appMeterOperationsAddProfileTask(const char *request, Callback_t callback);

S32 appMeterOperationsAddObisTask(const char *request, Callback_t callback);

S32 appMeterOperationsAddWriteTask(const char *request, Callback_t callback);

ERR_CODE_T appMeterOperationsIsTaskDone(S32 taskID);

ERR_CODE_T appMeterOperationsTaskIDFree(S32 taskID);

/* Example directives
"directives": [
{
"id" : "ReadoutDirective",
"directive": [
{
"operation": "setBaud",
"parameter": "300"
},
{
"operation": "setFraming",
"parameter": "7E1"
},
{
"operation": "sendData",
"parameter": "/?![0D][0A]"
},
{
"operation": "wait",
"parameter": "10"
},
{
"operation": "readData",
"parameter": "id"
},
{
"operation": "sendData",
"parameter": "[06]050[0D][0A]"
},
{
"operation": "setBaud",
"parameter": "9600"
},
{
"operation": "wait",
"parameter": "600"
},
{
"operation": "readData",
"parameter": "rawData"
}
]
}
]
*/


/*
* @brief Add a directive (full JSON). Stored under DIRECTIVE_MAIN_DIR; id taken from root "id" field.
* @return Zero-based index on success, or negative ERR_CODE_T value on failure
 */
S32 appMeterOperationsAddDirective(const char *directive);

/**
 * @brief Delete a directive
 * @param directiveID Directive ID, if NULL or * is given, all directives will be deleted
 * @return SUCCESS if the directive is deleted successfully, FAILURE otherwise
 */
RETURN_STATUS appMeterOperationsDeleteDirective(const char *directiveID);

/**
 * @brief Get the list of directives from the meter manager
 * @param directiveID Pointer to the directive ID
 * @return const char * Pointer to the directive list
 */
RETURN_STATUS appMeterOperationsGetDirective(const char *directiveID, char *directiveOut, size_t directiveOutSz);

/**
 * @brief Get a directive by index from the meter manager
 * @param index Index of the directive
 * @return const char * Pointer to the directive
 */
RETURN_STATUS appMeterOperationsGetDirectiveByIndex(U32 index, char *directiveOut, size_t directiveOutSz);

/**
 * @brief Get the count of directives from the meter manager
 * @return S32 Count of directives
 */
U32 appMeterOperationsGetDirectiveCount(void);

#endif /* __APP_METER_OPERATIONS_H__ */

/********************************* End Of File ********************************/
