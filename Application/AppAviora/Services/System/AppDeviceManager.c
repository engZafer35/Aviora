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
#include "Protocol_Config.h"

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
    retVal |= middMCUSPIInit();
    retVal |= middMCUI2CInit();
    retVal |= middMCUUartInit();
    retVal |= middMCUTimers();
    retVal |= middMcuRTCInit();

    DEBUG_INFO("\r\n\r\n");
    DEBUG_INFO(AVIORA_NAME_BANNER);
    DEBUG_INFO("******************* Aviora - Let Your Data Fly *******************\r\n");
    DEBUG_INFO("************** https://engzafer35.github.io/Aviora/ **************\r\n");
    DEBUG_INFO("************** https://github.com/engZafer35/Aviora **************\r\n");
    DEBUG_INFO("*************************** USER *********************************");
    DEBUG_INFO("User Name: %s", CUSTOMER_NAME);
    DEBUG_INFO("User Email: %s", CUSTOMER_EMAIL);

    DEBUG_INFO("\r\n[I]-> ************** initMcuCore return: %d **************\r\n", retVal);
    return SUCCESS;
}

/*
 * @brief init all device drivers
 */
static RETURN_STATUS initDeviceDrivers(void)
{
    RETURN_STATUS retVal = SUCCESS;
    DEBUG_VERBOSE("[I]-> Device Driver init Starting");

    /*
     * if you need to init a device driver, handle in this func.
     * use macro to plug in/out device to project. check ProjectConf.h file
     */

    DEBUG_INFO("[I]-> Device Driver init completed, result %d", retVal);
    return retVal;
}
/***************************** PUBLIC FUNCTIONS  ******************************/

RETURN_STATUS appFeedWdt(void)
{
    return SUCCESS;
}
#include <stdlib.h>
void appDevMngHwRestart(void)
{
    //TODO: stop all thread/task
    //TODO: stop feed wdt
    //todo: call hw reset func
#if defined(USE_FREERTOS)
    __disable_irq();

    HAL_NVIC_SystemReset();

    while(1); // güvenlik
#else
    exit(0);
#endif
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

        sleep(1000);
        sleep(1000);
        sleep(1000);
        sleep(1000);
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
