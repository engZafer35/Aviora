/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : MiddDigitalIOControl.C 
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG    (DISABLE)//(ENABLE)//
/********************************* INCLUDES ***********************************/
#include "Project_Conf.h"
#include "MiddDigitalIOControl.h"

#include "McuInterruptRegister.h"
/****************************** MACRO DEFINITIONS *****************************/

/*************************** FUNCTION PROTOTYPES ******************************/

/******************************* TYPE DEFINITIONS *****************************/


/********************************** VARIABLES *********************************/

static VUICallback gCbList[EN_IN_MAX_NUM];

/***************************** STATIC FUNCTIONS  ******************************/
void cbGpioExtInt(int pin)
{
    if ((INPUT_AC_PIN == pin) && (NULL != gCbList[EN_IN_AC_INPUT]))
    {
        gCbList[EN_IN_AC_INPUT](READ_AC_INPUT());
    }
    else if ((INPUT_SCAP_PIN == pin) && (NULL != gCbList[EN_IN_SCAP_STAT]))
    {
        gCbList[EN_IN_SCAP_STAT](READ_SCAP_INPUT());
    }
//    else if ((EXT_PCA_INT2_U16_Pin == pin) && (NULL != gCbList[EN_IN_PCA_U16_INT]))
//    {
//        gCbList[EN_IN_PCA_U16_INT](FALSE); //Don't read gpio, just falling edge interrupt
//    }
//    else if ((EXT_INT1_PCA_U7_Pin == pin) && (NULL != gCbList[EN_IN_PCA_U7_INT]))
//    {
//        gCbList[EN_IN_PCA_U7_INT](FALSE); //Don't read gpio, just falling edge interrupt
//    }
//    else if ((EXT_INT4_PCA_U8_Pin == pin) && (NULL != gCbList[EN_IN_PCA_U8_INT]))
//    {
//        gCbList[EN_IN_PCA_U8_INT](FALSE); //Don't read gpio, just falling edge interrupt
//    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
/**
 * @brief  init digital IO
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOInit(void)
{
    RETURN_STATUS retVal = SUCCESS;

    retVal = drvIntRegister(cbGpioExtInt, GPIO_AC_INT_CHNL);
    /* drvIntRegister(cbGpioExtInt, GPIO_SCAP_INT_CHNL); -> AC and SCAP inputs are in the same ext interrupt channel, so don't need to call this line */

    /** Clear all outputs */
    middIOWrite(EN_OUT_GSM_LED_1, DISABLE);
    middIOWrite(EN_OUT_GSM_LED_2, DISABLE);
    middIOWrite(EN_OUT_GSM_LED_3, DISABLE);
    middIOWrite(EN_OUT_GSM_LED_4, DISABLE);
    middIOWrite(EN_OUT_GSM_LED_5, DISABLE);
    middIOWrite(EN_OUT_GSM_CONN_LED,     DISABLE);
    middIOWrite(EN_OUT_GSM_INTERNET_LED, DISABLE);

    middIOWrite(EN_OUT_JOB_STATUS_LED, DISABLE);
    middIOWrite(EN_OUT_POWER_LED,      DISABLE);

    return retVal;
}

/**
 * @brief  listen a interrupt input(Check whick input support interrupt)
 * @param  void(int) callback pointer
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOIntListen(EN_IN_LIST in, VUICallback cb)
{
    RETURN_STATUS retVal = FAILURE;

    if ((EN_IN_MAX_NUM > in) && IS_SAFELY_PTR(cb))
    {
        retVal = SUCCESS;
        gCbList[in] = cb;
    }

    return retVal;
}

RETURN_STATUS middIOIntStopListen(EN_IN_LIST in)
{
    RETURN_STATUS retVal = FAILURE;

    if (EN_IN_MAX_NUM > in)
    {
        retVal = SUCCESS;
        gCbList[in] = NULL;
    }

    return retVal;
}

/**
 * @brief  set output
 * @param  output channel list
 * @param  ENABLE active, DISABLE passif
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */

RETURN_STATUS middIOWrite(EN_OUT_LIST out, BOOL stat)
{
    RETURN_STATUS retVal = SUCCESS;

    switch(out)
    {
        case EN_OUT_JOB_STATUS_LED:   JOB_STATUS_LED(stat);   break;
        case EN_OUT_POWER_LED:        POWER_STATUS_LED(stat); break;

        case EN_OUT_GSM_LED_1:        GSM_LEVEL_LED_1(stat);  break;
        case EN_OUT_GSM_LED_2:        GSM_LEVEL_LED_2(stat);  break;
        case EN_OUT_GSM_LED_3:        GSM_LEVEL_LED_3(stat);  break;
        case EN_OUT_GSM_LED_4:        GSM_LEVEL_LED_4(stat);  break;
        case EN_OUT_GSM_LED_5:        GSM_LEVEL_LED_5(stat);  break;

        case EN_OUT_GSM_CONN_LED:     GSM_CONN_LED(stat);         break;
        case EN_OUT_GSM_INTERNET_LED: GSM_INTERNET_LED(stat);     break;

        default: retVal = FAILURE;
    }

    return retVal;
}

/**
 * @brief  read input
 * @param  input channel list
 * @return 1 or 0,
 */
BOOL middIORead(EN_IN_LIST in)
{
    BOOL retVal = 0;
    switch(in)
    {
        case EN_IN_AC_INPUT:    retVal = READ_AC_INPUT(); 	break;
        case EN_IN_SCAP_STAT:   retVal = READ_SCAP_INPUT(); break;

        default: break;
    }

    return retVal;
}

/**
 * @brief  toogle status output
 * @param  ID
 * @param  status, enable or disable
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middIOCtrlToggleLed(EN_OUT_LIST out)
{
    RETURN_STATUS retVal = SUCCESS;

    switch(out)
    {
        case EN_OUT_JOB_STATUS_LED: TOGGLE_JOB_STATUS_LED();     break;
        case EN_OUT_POWER_LED:      TOGGLE_POWER_STATUS_LED();   break;

        case EN_OUT_GSM_LED_1:      TOGGLE_GSM_LEVEL_LED_1();    break;
        case EN_OUT_GSM_LED_2:      TOGGLE_GSM_LEVEL_LED_2();    break;
        case EN_OUT_GSM_LED_3:      TOGGLE_GSM_LEVEL_LED_3();    break;
        case EN_OUT_GSM_LED_4:      TOGGLE_GSM_LEVEL_LED_4();    break;
        case EN_OUT_GSM_LED_5:      TOGGLE_GSM_LEVEL_LED_5();    break;

        case EN_OUT_GSM_CONN_LED:       TOGGLE_GSM_CONN();           break;
        case EN_OUT_GSM_INTERNET_LED:   TOGGLE_GSM_INTERNET();       break;

        default: retVal = FAILURE;
    }

    return retVal;
}
/******************************** End Of File *********************************/
