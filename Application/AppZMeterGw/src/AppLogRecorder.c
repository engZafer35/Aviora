/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 17:46:08
* #File Name    : AppLogRecorder.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppLogRecorder.h"

#include <string.h>
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
static struct
{
    LogRecInterface srvIFace;
    S32 loggerID;
    char serviceName[16];
}services[MAX_SERVICE_NUMBER];
/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appLogRecInit(void)
{
    U32 i;
    for (i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        services[i].loggerID = -1; //no id
    }

    return SUCCESS;
}

RETURN_STATUS appLogRecRegister(LogRecInterface *logger, const char *srvName, S32 *loggerID)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    for (i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        if ('\0' == services[i].serviceName[0])
        {
            services[i].srvIFace = *logger;
            strcpy(services[i].serviceName, srvName);
            services[i].loggerID = i;
            *loggerID = i;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appLogRecUnregister(const char *srvName, S32 loggerID)
{
    RETURN_STATUS retVal = FAILURE;

    if (-1 != loggerID) //check if it is valid ID
    {
        if (0 == strcmp(srvName, services[loggerID].serviceName))
        {
            memset(services[loggerID].serviceName, 0, sizeof(services[loggerID].serviceName));
            services[loggerID].loggerID = -1; //no id

            retVal = SUCCESS;
        }
    }
    return retVal;
}

S32 appLogRec(S32 loggerID, const char *logStr)
{
    S32 retVal = -1;

    if (-1 != loggerID) //check if it is valid ID
    {
        retVal = services[loggerID].srvIFace.writeFunc(logStr);
    }

    return retVal;
}

S32 appLogRecRead(S32 loggerID, const char *str, U32 size)
{
    S32 retVal = -1;

    if (-1 != loggerID) //check if it is valid ID
    {
        retVal = services[loggerID].srvIFace.readFunc(str, size);
    }

    return retVal;
}

S32 appLogRecGetLoggerID(const char *srvName)
{
    U32 retVal = -1; //no id
    U32 i;

    for (i = 0; i < MAX_SERVICE_NUMBER; i++)
    {
        if (0 == strcmp(srvName, services[i].serviceName))
        {
            retVal = i; //load logger ID
        }
    }

    return retVal;
}

/**
 * fileNum 0, current file
 * fileNum 0-0xffffffff last X file,
 * fileNum 0xffffffff all log file
 */
RETURN_STATUS appLogRecSendLog(S32 loggerID, EN_SRV_LOG_SEND sendType, U32 fileNum)
{
    //when this function is called, lock log writer.
    return SUCCESS;
}
/******************************** End Of File *********************************/
