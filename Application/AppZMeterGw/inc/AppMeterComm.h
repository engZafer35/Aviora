/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Aug 27, 2024 - 10:12:44 AM
* #File Name    : AppMeterComm.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_METERCOMM_H__
#define __APP_METERCOMM_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
//UART driver abstraction layer
typedef error_t (*mc_init)(void);
typedef void (*UartEnableIrq)(void);
typedef void (*UartDisableIrq)(void);
typedef void (*UartStartTx)(void);

typedef struct
{
    RETURN_STATUS (*mSend) (const U8 *buff, U32 bufLeng, U32 timeout);
    S32 (*mRcv) (char *buff, int bufLeng, int timeout);
} MeterCommLine;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
RETURN_STATUS appMeterCommInit(MeterCommLine *commLine);




#endif /* __APPLICATION_APPZMETERGW_SRC_APPMETERCOMM_H__ */

/********************************* End Of File ********************************/
