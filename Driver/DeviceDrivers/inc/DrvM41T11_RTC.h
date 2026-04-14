/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : DrvM41T11_RTC.h
*******************************************************************************/

/******************************************************************************
*                                Pin Table Description
*     Pin
*      1   -   OSCI    -  Oscillator input
*      2   -   OCSO    -  Oscillator output
*      3   -   Vbat    -  Battery supply voltage
*      4   -   Vss     -  Ground
*      5   -   SDA     -  Serial data address input/output
*      6   -   SCL     -  Serial clock
*      7   -   FT/OUT  -  Frequency test/output driver (open drain)
*      8   -   Vcc     -  Supply voltage
*
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __DRV_M41T11_RTC_H__
#define __DRV_M41T11_RTC_H__
/*********************************INCLUDES*************************************/
#include "Global_Definitions.h"
/******************************MACRO DEFINITIONS*******************************/
/** driver functions for external RTC. Each Rtch driver has its own implementation */
#define EXT_RTC_INIT_FUNC(x)      drvM41T11Init(x)
#define EXT_RTC_GET_TIME_FUNC(t)  drvM41T11GetTime(t)
#define EXT_RTC_SET_TIME_FUNC(t)  drvM41T11SetTime(t)
/** end of driver functions for external RTC */
/*******************************TYPE DEFINITIONS ******************************/
/**
 * @brief m41t11 driver time structure
 */
typedef struct _RtcStr_t
{
    U8 sec;     /* Seconds parameter, from 00 to 59 */
    U8 min;     /* Minutes parameter, from 00 to 59 */
    U8 hour;    /* Hours parameter, 24Hour mode, 00 to 23 */
    U8 wday;    /* Day in a week, from 1 to 7 */
    U8 mday;    /* Date in a month, 1 to 31 */
    U8 mon;     /* Month in a year, 1 to 12 */
    U16 year;   /* Year parameter, 2000 to 3000 */
}RtcStr_t;

 typedef struct _RtcI2c_t
 {
     U32 devAddr;
     RETURN_STATUS (*writeI2C) (U32 devAddr, U32 memAdr, U8* buff, U32 bufLeng);
     RETURN_STATUS (*readI2C)  (U32 devAddr, U32 memAdr, U8* buff, U32 bufLeng);
 }RtcI2c_t;
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
* @brief  init M41T11
* @param  hardware i2c, driver will copy i2c, user can delete own structure
* @return if everything is OK, return SUCCES
*         otherwise return FAILURE
 */
RETURN_STATUS drvM41T11Init(const RtcI2c_t *i2c);

/**
 * @brief  get time
 * @param  RtcStr_t pointer
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS drvM41T11GetTime(RtcStr_t *getTime);

/**
 * @brief  set time
 * @param  RtcStr_t pointer
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS drvM41T11SetTime(const RtcStr_t *setTime);

#endif /* __DRV_M41T11_RTC_H__ */

/********************************* End Of File ********************************/
