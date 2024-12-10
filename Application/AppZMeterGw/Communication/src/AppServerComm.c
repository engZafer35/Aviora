/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jul 10, 2024 - 11:51:31 AM
* #File Name    : AppServerComm.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppServerComm.h"
#include "AppVikoProtocol.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
SERVER_PROTOCOL gs_currentProtocol;
/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appServerCommInit(SERVER_PROTOCOL protocol, const char *servIP, int port)
{
    RETURN_STATUS retVal = SUCCESS;

    switch (protocol)
    {
        case EN_SERVER_PROTOCOL_TEDAS:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_VIKO:
        {
            appVikoInit(servIP, port);
            break;
        }

        case EN_SERVER_PROTOCOL_ZMETER:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_NONE:
        case EN_SERVER_PROTOCOL_MAX_NUM:
            retVal = FAILURE; break;
    }

    if (SUCCESS == retVal)
    {
        gs_currentProtocol = protocol;
    }

    return retVal;
}
RETURN_STATUS appServerSendPacket(enum MESSAGES msg, const char *buff, U32 buffLeng)
{
    RETURN_STATUS retVal = SUCCESS;
    switch (gs_currentProtocol)
    {

        case EN_SERVER_PROTOCOL_TEDAS:
        {

            break;
        }

        case EN_SERVER_PROTOCOL_VIKO:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_ZMETER:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_NONE:
        case EN_SERVER_PROTOCOL_MAX_NUM:
            retVal = FAILURE; break;
    }

    return retVal;
}

RETURN_STATUS appServerCommStart(void)
{
    RETURN_STATUS retVal = SUCCESS;

    switch (gs_currentProtocol)
    {
        case EN_SERVER_PROTOCOL_TEDAS:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_VIKO:
        {
            appVikoTaskStart();
            break;
        }

        case EN_SERVER_PROTOCOL_ZMETER:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_NONE:
        case EN_SERVER_PROTOCOL_MAX_NUM:
            retVal = FAILURE; break;
    }

    return retVal;
}

RETURN_STATUS appServerCommStop(void)
{
    RETURN_STATUS retVal = SUCCESS;

    switch (gs_currentProtocol)
    {
        case EN_SERVER_PROTOCOL_TEDAS:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_VIKO:
        {
            appVikoTaskStop();
            break;
        }

        case EN_SERVER_PROTOCOL_ZMETER:
        {
            break;
        }

        case EN_SERVER_PROTOCOL_NONE:
        case EN_SERVER_PROTOCOL_MAX_NUM:
            retVal = FAILURE; break;
    }

    return retVal;
}

/******************************** End Of File *********************************/
