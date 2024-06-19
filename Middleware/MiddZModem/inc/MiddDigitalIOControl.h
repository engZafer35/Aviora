/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : MiddDigitalIOControl.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MIDD_DIGITAL_IO_CONTROL_H__
#define __MIDD_DIGITAL_IO_CONTROL_H__
/*********************************INCLUDES*************************************/
#include "Global_Definitions.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
typedef enum _EN_IN_LIST
{
    EN_IN_COVER_ALERT,
    EN_IN_BODY_ALERT,

    EN_IN_AC_INPUT,
    EN_IN_SCAP_STAT,
    EN_IN_EA_INPUT_1,
    EN_IN_EA_INPUT_2,

    EN_IN_DI_1,
    EN_IN_DI_2,
    EN_IN_DI_3,
    EN_IN_DI_4,
    EN_IN_DI_5,
    EN_IN_DI_6,

	EN_IN_MAX_NUM

}EN_IN_LIST;

typedef enum _EN_OUT_LIST
{
    EN_OUT_JOB_STATUS_LED,
    EN_OUT_POWER_LED,

    EN_OUT_GSM_LED_1,
    EN_OUT_GSM_LED_2,
    EN_OUT_GSM_LED_3,
    EN_OUT_GSM_LED_4,
    EN_OUT_GSM_LED_5,

    EN_OUT_GSM_CONN_LED,
    EN_OUT_GSM_INTERNET_LED,

    EN_OUT_SCAP_CHARGE,

    EN_OUT_MAX_NUMBER,

}EN_OUT_LIST;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief  init digital IO
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOInit(void);

/**
 * @brief  listen a interrupt input(Check which input support interrupt)
 * @param  input channel
 * @param  callback function pointer
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOIntListen(EN_IN_LIST in, VUICallback cb);

/**
 * @brief  stop listen an input channel
 * @param  input channel
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOIntStopListen(EN_IN_LIST in);

/**
 * @brief  set output
 * @param  output channel list
 * @param  ENABLE active, DISABLE passive
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOWrite(EN_OUT_LIST out, BOOL stat);

/**
 * @brief  set all output together
 * @param  ENABLE active, DISABLE passive
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOWriteAll(BOOL stat);

/**
 * @brief  read input
 * @param  input channel list
 * @return 1 or 0, (TRUE, FALSE)
 */
BOOL middIORead(EN_IN_LIST in);

/**
 * @brief  Toggle output
 * @param  output channel list
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOCtrlToggleLed(EN_OUT_LIST out);

#endif /* __MIDD_DIGITAL_IO_CONTROL_H__ */

/********************************* End Of File ********************************/
