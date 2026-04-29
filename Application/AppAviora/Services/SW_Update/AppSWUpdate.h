/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 14 Mar 2026 - 18:21:51
* #File Name    : AppSWUpdate.h
*******************************************************************************/
/******************************************************************************
* Example OTA update startup call (uncomment and set real server/path if desired)
* appSwUpdateInit("test.rebex.net", 21, "demo", "password", "readme.txt", "/tmp/readme.txt");
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_SW_UPDATE_H__
#define __APP_SW_UPDATE_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

/******************************MACRO DEFINITIONS*******************************/

#ifdef __cplusplus
extern "C" {
#endif

/*******************************TYPE DEFINITIONS ******************************/
/**
 * @brief Enumeration for SW update results
 * The result is shared via data bus with other components
 */
typedef enum 
{
    EN_SW_UPDATE_RESULT_IDLE,
    EN_SW_UPDATE_RESULT_SUCCESS,
    EN_SW_UPDATE_RESULT_FAILED,
    EN_SW_UPDATE_RESULT_INVALID_PARAM,
} SW_UPDATE_RESULT;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Init SW update and start ftp download task.
 * @param serverIp FTP server IP or hostname string
 * @param serverPort FTP server port
 * @param username FTP username
 * @param password FTP password
 * @param remoteFilePath Remote file path on FTP server
 * @param localFilePath Local destination file path
 * @return SUCCESS if task started, otherwise FAILURE
 */
RETURN_STATUS AppSwUpdateInit(const char *serverIp,
                              U16 serverPort,
                              const char *username,
                              const char *password,
                              const char *remoteFilePath,
                              const char *localFilePath);

#ifdef __cplusplus
}
#endif

#endif /* __APP_SW_UPDATE_H__ */

/********************************* End Of File ********************************/
