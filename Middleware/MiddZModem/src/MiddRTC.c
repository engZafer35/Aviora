/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : March 19, 2026 - 14:46:00
* #File Name    : MiddRTC.c 
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include <string.h>

#include "Project_Conf.h"
#include "MiddRTC.h"

/****************************** MACRO DEFINITIONS *****************************/

/*************************** FUNCTION PROTOTYPES ******************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
static RETURN_STATUS i2cWriteFunc(U32 devAdr, U32 memAdr, U8* buff, U32 bufLeng)
{
    RETURN_STATUS retVal = FAILURE;

    if (DRV_RET_SUCCESS == I2C_WRITE(EXT_RTC_I2C_LINE, devAdr, memAdr, buff, bufLeng))
    {
        retVal = SUCCESS;
    }

    return retVal;
}

static RETURN_STATUS i2cReadFunc(U32 devAdr, U32 memAdr, U8* buff, U32 bufLeng)
{
    RETURN_STATUS retVal = FAILURE;

    if (DRV_RET_SUCCESS == I2C_READ(EXT_RTC_I2C_LINE, devAdr, memAdr, buff, bufLeng))
    {
        retVal = SUCCESS;
    }

    return retVal;
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS middRtcExtInit(void)
{    
    RtcI2c_t rtcI2c;

    rtcI2c.devAddr = EXT_RTC_I2C_ADDR;
    rtcI2c.read    = i2cReadFunc;
    rtcI2c.write   = i2cWriteFunc;

    return EXT_RTC_INIT_FUNC(&rtcI2c);    
}

RETURN_STATUS middRtcExtGetTime(RtcStr_t *t)
{
    RETURN_STATUS retVal;

    if (IS_SAFELY_PTR(t))
    {
        retVal = EXT_RTC_GET_TIME_FUNC(t);
    }

    return retVal;
}

RETURN_STATUS middRtcExtSetTime(const RtcStr_t *t)
{
    RETURN_STATUS retVal;

    if (IS_SAFELY_PTR(t))
    {
        retVal = EXT_RTC_SET_TIME_FUNC(t);
    }

    return retVal;
}

/**************** Internal RTC Functions ****************/
RETURN_STATUS middRtcIntInit(void)
{
    return INT_RTC_INIT_FUNC();
}

RETURN_STATUS middRtcIntGetTime(RtcStr_t *t)
{
    return INT_RTC_GET_TIME_FUNC(t);
}

RETURN_STATUS middRtcIntSetTime(const RtcStr_t *t)
{
    return INT_RTC_SET_TIME_FUNC(t);
}

/******************************** End Of File *********************************/
