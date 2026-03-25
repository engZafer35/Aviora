/******************************************************************************
* #Author       : Zafer Satilmis
* #File Name    : Cus_loggerService_Conf.c
* #Customer     : ZD_0101
* #Date         : 2026-03-25 16:07:07
******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: ZD_0101
* Generated from customer logger_config.json via generate_logger_service.py.
******************************************************************************/

#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "Cus_loggerService_Conf.h"
#include "Project_Conf.h"
#include "AppLogRecorder.h"

#include LOGGER_SYS_FS_PATH
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

S32 g_sysLoggerID;

/******************************* PUBLIC FUNCTIONS  *****************************/

RETURN_STATUS appLogStartLoggers(void)
{
    RETURN_STATUS retVal = SUCCESS;

    char loggerSysName[] = LOGGER_SYS_NAME;

    logRecConf_t sysLoggerConf;

    sysLoggerConf.openFunc  = LOGGER_SYS_OPEN_FUNC;
    sysLoggerConf.writeFunc = LOGGER_SYS_WRITE_FUNC;
    sysLoggerConf.readFunc  = LOGGER_SYS_READ_FUNC;
    sysLoggerConf.closeFunc = LOGGER_SYS_CLOSE_FUNC;
    sysLoggerConf.fileSize  = LOGGER_SYS_FILE_SIZE;
    sysLoggerConf.logPath   = LOGGER_SYS_LOG_PATH;
    sysLoggerConf.totalLogSize = LOGGER_SYS_TOTAL_LOG_SIZE;
    sysLoggerConf.timeStamp = LOGGER_SYS_TIME_INFO_USE;
    sysLoggerConf.timeStampFormat = LOGGER_SYS_TIME_INFO_FORMAT;

    retVal |= appLogRecRegister(&sysLoggerConf, loggerSysName, &g_sysLoggerID);

    return retVal;
}

/******************************** End Of File *********************************/
