/******************************************************************************
* #Author       : Zafer Satilmis
* #File Name    : Cus_LoggerService_Conf.c
* #Customer     : ZD_2622
* #Date         : 2026-04-08 13:38:44
******************************************************************************/
/******************************************************************************
* AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
* Customer Name: ZD_2622
* Generated from customer logger_config.json via generate_logger_service.py.
******************************************************************************/

#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "Cus_LoggerService_Conf.h"
#include "Project_Conf.h"
#include "AppLogRecorder.h"

#include LOGGER_SYS_FS_PATH
#include LOGGER_PROT_FS_PATH
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

S32 g_sysLoggerID;
S32 g_protLoggerID;

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

    char loggerProtName[] = LOGGER_PROT_NAME;

    logRecConf_t protocolLoggerConf;

    protocolLoggerConf.openFunc  = LOGGER_PROT_OPEN_FUNC;
    protocolLoggerConf.writeFunc = LOGGER_PROT_WRITE_FUNC;
    protocolLoggerConf.readFunc  = LOGGER_PROT_READ_FUNC;
    protocolLoggerConf.closeFunc = LOGGER_PROT_CLOSE_FUNC;
    protocolLoggerConf.fileSize  = LOGGER_PROT_FILE_SIZE;
    protocolLoggerConf.logPath   = LOGGER_PROT_LOG_PATH;
    protocolLoggerConf.totalLogSize = LOGGER_PROT_TOTAL_LOG_SIZE;
    protocolLoggerConf.timeStamp = LOGGER_PROT_TIME_INFO_USE;
    protocolLoggerConf.timeStampFormat = LOGGER_PROT_TIME_INFO_FORMAT;

    retVal |= appLogRecRegister(&protocolLoggerConf, loggerProtName, &g_protLoggerID);

    return retVal;
}

/******************************** End Of File *********************************/
