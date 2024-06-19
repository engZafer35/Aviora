/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 26 Mar 2024 - 15:51:52
* #File Name    : MiddGsmModule.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MIDD_GSM_MODULE_H__
#define __MIDD_GSM_MODULE_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef enum
{
    EN_GSM_WORKING_MODE_DATA_MODE, //data mode
    EN_GSM_WORKING_MODE_COMM_MODE //communication mode
}EN_GSM_WORKING_MODE;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS middGsmInitModule(void);

RETURN_STATUS middGsmPPPConnection(BOOL stat); //TRUE create ppp connection, 0 close ppp connection

RETURN_STATUS middGsmPower(BOOL stat); //1 power up, 0 power donw

RETURN_STATUS middGsmSwReset(void);

RETURN_STATUS middGsmATCommand(const char *atMsg);

RETURN_STATUS middGsmGetConnStatus(void);

U32           middGsmGetSignalLevel(void);

RETURN_STATUS middGsmSetWorkingMode(EN_GSM_WORKING_MODE mode);

RETURN_STATUS middGsmGetWorkingMode(void);

#endif /* __MIDDLEWARE_MIDDZMODEM_INC_MIDDGSMMODULE_H__ */

/********************************* End Of File ********************************/
