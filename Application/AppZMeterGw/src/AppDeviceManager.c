/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 22 Mar 2024 - 13:34:41
* #File Name    : AppDeviceManager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (ENABLE)
/********************************* INCLUDES ***********************************/
#include "AppDeviceManager.h"

#include "MiddMCUCore.h"
#include "MiddDigitalIOControl.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
/*
 * @brief init all mcu peripherals
 */
static RETURN_STATUS initMcuCore(void)
{
    RETURN_STATUS retVal = SUCCESS;

    retVal |= middMCUClockInit(); //firstly init clock and system
    retVal |= middMCUGpioInit();
    retVal |= middMCUI2CInit();
    retVal |= middMCUUartInit();
    retVal |= middMCUTimers();

//    retVal |= middUSB();

    DEBUG_INFO(" ->[I] initMcuCore return: %d", retVal);
    return SUCCESS;
}

/*
 * @brief init all device drivers
 */
static RETURN_STATUS initDeviceDrivers(void)
{
    RETURN_STATUS retVal = SUCCESS;
    DEBUG_VERBOSE(" ->[I] Device Driver Starting");

    /*
     * if you need to init a device driver, handle in this func.
     * use macro to plug in/out device to project. check ProjectConf.h file
     */

    DEBUG_INFO(" ->[I] Device Driver completed, result %d", retVal);
    return retVal;
}
/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appFeedWdt(void)
{
    return SUCCESS;
}

void appDevMngHwRestart(void)
{
    //TODO: stop all thread/task
    //TODO: stop feed wdt
    //todo: call hw reset func

    while(1) /* wait until hw reset */
        ;
}

void appDevMngSwRestart(void)
{

}

RETURN_STATUS appDevMngInitHwUnits(void)
{
    RETURN_STATUS retVal = FAILURE;

    if (SUCCESS == initMcuCore())
    {
        //use fprintf to show system info for every time.
        DEBUG_INFO("##--- > Board File: %s - Board Name: %s - Board Version: %s Board MCU: %s-%s", BOARD_FILE_NAME, BOARD_NAME, BOARD_VERSION, MCU_PART_NUM, MCU_CORE);
        DEBUG_INFO("##--- > SW Version %d.%d.%d \n\r", SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_BUGFX);

        retVal = initDeviceDrivers();

        middIOWrite(EN_OUT_POWER_LED, ENABLE);
    }

    return retVal;
}

RETURN_STATUS appDevMngCloseHw(HARDWARE_UNITS unit)
{
    return SUCCESS;
}

RETURN_STATUS appDevMngStartHw(HARDWARE_UNITS unit)
{
    return SUCCESS;
}

RETURN_STATUS appDevMngRestartHw(HARDWARE_UNITS unit)
{
    return SUCCESS;
}
/******************************** End Of File *********************************/
