/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 17:46:00
* #File Name    : AppLogRecorder.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_LOG_RECORDER_H__
#define __APP_LOG_RECORDER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

#define MAX_SERVICE_NUMBER (2) //move this param to configuration file

#define FTP_SERVER  (ENABLE) //(DISABLE) /* If ftp server exists, enable this macro

#define ALL_LOG_FILES   (0xFFFFFFFF)
/*******************************TYPE DEFINITIONS ******************************/

typedef int (*WriteLog) (const char *logStr);
typedef int (*ReadLog) (const char *logStr, int size);

/**
 * Service logger interface
 */
typedef struct
{
    U32 fileSize;     /* One service log file size, byte */
    U32 totalLogSize; /* Total service log area size, byte */

    WriteLog writeFunc;
    ReadLog readFunc;

    const char *logPath;

}LogRecInterface;

typedef enum
{
    EN_SRV_LOG_SEND_FTP,
    EN_SRV_LOG_SERIAL_PORT,

}EN_SRV_LOG_SEND;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appLogRecInit(void);

RETURN_STATUS appLogRecRegister(LogRecInterface *logger, const char *srvName, S32 *loggerID);

RETURN_STATUS appLogRecUnregister(const char *srvName, S32 loggerID);

S32 appLogRec(S32 loggerID, const char *logStr);

S32 appLogRecRead(S32 loggerID, const char *str, U32 size);

S32 appLogRecGetLoggerID(const char *srvName);

/**
 * fileNum 0, current file
 * fileNum 0-0xffffffff last X file,
 * fileNum 0xffffffff all log file
 */
RETURN_STATUS appLogRecSendLog(S32 loggerID, EN_SRV_LOG_SEND sendType, U32 fileNum);

#endif /* __APP_LOG_RECORDER_H__ */

/********************************* End Of File ********************************/
