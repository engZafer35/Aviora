/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
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
#include "AppTimeService.h"
#include "LoggerService_Config.h"
/******************************MACRO DEFINITIONS*******************************/

#define MAX_SERVICE_NUMBER (2) //move this param to configuration file
#define LOGGER_QUEUE_SIZE (10)
#define LOGGER_LOG_MAX_LENGTH (128)

#define FTP_SERVER  (ENABLE) //(DISABLE) /* If ftp server exists, enable this macro

#define ALL_LOG_FILES   (0xFFFFFFFF)
/*******************************TYPE DEFINITIONS ******************************/
/** 
 * Logger interface function pointers 
 */
typedef void *(*OpenLog) (const char *path, unsigned int mode);
typedef RETURN_STATUS (*WriteLog) (const void *file, const char *data, size_t length);
typedef RETURN_STATUS (*ReadLog)  (const void *file, char *data, size_t size, size_t *outLength);
typedef void (*CloseLogFile)(void *file);

/**
 * Service logger interface
 */
typedef struct
{
    U32 fileSize;     /* One service log file size, byte */
    U32 totalLogSize; /* Total service log area size, byte */

    OpenLog openFunc;
    WriteLog writeFunc;
    ReadLog readFunc;
    CloseLogFile closeFunc;
    
    const char *logPath;
    BOOL timeStamp;
    APP_TIME_STRING_FORMAT timeStampFormat;

}logRecConf_t;

typedef enum
{
    EN_SRV_LOG_SEND_FTP,
    EN_SRV_LOG_SERIAL_PORT,

}EN_SRV_LOG_SEND;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Initialize the log recorder module
 * @return SUCCESS on success, FAILURE on failure
 */
RETURN_STATUS appLogRecInit(void);

/**
 * @brief Start the logger services
 * @return SUCCESS on success, FAILURE on failure
 */
RETURN_STATUS appLogStartLoggers(void);

/**
 * @brief Register a logger service
 * @param logger Logger interface structure
 * @param srvName Service name
 * @param loggerID Pointer to store the assigned logger ID
 * @return SUCCESS on success, FAILURE on failure
 */
RETURN_STATUS appLogRecRegister(logRecConf_t *logger, const char *srvName, S32 *loggerID);

/**
 * @brief Unregister a logger service
 * @param srvName Service name
 * @param loggerID Logger ID to unregister
 * @return SUCCESS on success, FAILURE on failure
 */
RETURN_STATUS appLogRecUnregister(const char *srvName, S32 loggerID);

/**
 * @brief Send log data to logger
 * @param loggerID Logger ID obtained from registration
 * @param logStr Log string to send
 * @return SUCCESS on success, FAILURE on failure
 */
RETURN_STATUS appLogRec(S32 loggerID, const char *logStr);

/**
 * @brief Read log data from logger
 * @param loggerID Logger ID obtained from registration
 * @param str Buffer to store the read log data
 * @param size Size of the buffer
 * @return Number of bytes read on success, -1 on failure
 */
S32 appLogRecRead(S32 loggerID, const char *str, U32 size);

/**
 * @brief Get logger ID by service name
 * @param srvName Service name  
 * @return Logger ID if found, -1 if not found or input is invalid
 * @note The caller should ensure that the service name is valid and registered before calling this function
 */
S32 appLogRecGetLoggerID(const char *srvName);

#endif /* __APP_LOG_RECORDER_H__ */

/********************************* End Of File ********************************/