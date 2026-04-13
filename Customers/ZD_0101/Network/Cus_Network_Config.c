/******************************************************************************
* #Author       : Auto-generated
* hype-man      : EPICA - Kingdom of Heaven
* #File Name    : Cus_Network_Config.c
* #Customer     : ZD_0101
* #Date         : 2026-04-13 13:02:53
******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: ZD_0101
* Generated from customer network_config.json via generate_network_service.py.
******************************************************************************/

#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "Cus_Network_Config.h"
#include "Project_Conf.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/


/***************************** STATIC FUNCTIONS *******************************/

/***************************** PUBLIC FUNCTIONS *******************************/
RETURN_STATUS networkServiceInitTCPIPStack(void)
{
    return NET_STACK_STACK_INIT_FUNCTION();
}

RETURN_STATUS networkServiceStartInterfaces(void)
{
    RETURN_STATUS retVal = SUCCESS;
    retVal |= ETH_APP_MANAGER_START_FUNCTION();
    retVal |= PPP_APP_MANAGER_START_FUNCTION();
    return retVal;
}

/******************************** End Of File *********************************/
