/******************************************************************************
* #Author       : Zafer Satilmis
* hype-man      : Epica - Cry For The Moon
* #Revision     : 1.0
* #Date         : 13 Apr 2026 - 00:223:02
* #File Name    : AppCycloneStack.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppCycloneStack.h"
#include "Customers/NetworkService_Config.h"

#include "net_config.h"

#include "AppLogRecorder.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/


/********************************** VARIABLES *********************************/


/***************************** STATIC FUNCTIONS  ******************************/


/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appCycloneTcpIpStackInit(void)
{
    RETURN_STATUS retVal = SUCCESS;

    //Cyclone TCP/IP stack initialization
    if(NO_ERROR != netInit())
    {
        retVal = FAILURE;
        DEBUG_ERROR("Failed to initialize Cyclone TCP stack!");
        APP_LOG_REC(g_sysLoggerID, "Failed to initialize Cyclone TCP stack!");
    }

    return retVal;
}

/******************************** End Of File *********************************/
