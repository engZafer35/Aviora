/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 15:08:07
* #File Name    : AppConfigManager.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_CONFIG_MANAGER_H__
#define __APP_CONFIG_MANAGER_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/
#define DEFAULT_CONF_PATH "confFile.cnf"
/*******************************TYPE DEFINITIONS ******************************/
typedef enum
{
    EN_CONF_SNTP_IP,
    EN_CONF_SNTP_PORT,
    EN_CONF_APN,
    EN_CONF_SERIAL_NUMBER,
    EN_CONF_SW_VERSION,
    EN_CONF_HW_VERSION,

    EN_CONF_COUNTRY,

    EN_CONF_MAX_NUMBER
}EN_CONF_LIST;


// json string name may be used instead of enum list.


/**
 * https://www.objgen.com/json?demo=true use this web page to create json conf file
 */
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appConfInit(const char *confPath);

RETURN_STATUS appConfUpdate(EN_CONF_LIST conf, const void *pdata);

RETURN_STATUS appConfGet(EN_CONF_LIST conf, void *pdata);

RETURN_STATUS appConfDefault(void); // this function delete all changes and load default values, like reset all system.

#endif /* __APP_CONFIG_MANAGER_H__ */

/********************************* End Of File ********************************/
