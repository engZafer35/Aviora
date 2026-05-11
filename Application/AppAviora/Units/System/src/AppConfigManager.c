/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 15:08:16
* #File Name    : AppConfigManager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppConfigManager.h"
#include "AppGlobalVariables.h"
#include "ZDJson.h"

#include "fs_port.h"
/****************************** MACRO DEFINITIONS *****************************/
#define DEV_CONF_FILE "devConfFile.dat"

/*
{
  "avioraSerialNum": "ZD*****" //MAX 15 byte
}
*/
/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
static char gs_devSerialStr[16];
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appConfInit(const char *confPath)
{
    RETURN_STATUS retVal = FAILURE;
    char confJsonBuff[256] = "";
    U32 rleng = 0;

    g_devSerial = NULL;
    FsFile *f = fsOpenFile((char *)DEV_CONF_FILE, FS_FILE_MODE_READ);
    if (f != NULL)
    {
        U8 val = 0; size_t got = 0;
        (void)fsReadFile(f, confJsonBuff, sizeof(confJsonBuff), &rleng);
        fsCloseFile(f);

        if (TRUE == ZDJson_GetStringValue(confJsonBuff, "avioraSerialNum", gs_devSerialStr, sizeof(gs_devSerialStr)))
        {
            g_devSerial = gs_devSerialStr;

            DEBUG_WARNING("[W]-> From Device conf, Device Serial Number %s", g_devSerial);
            retVal = SUCCESS;
        }
        else
        {
            DEBUG_ERROR("[E]-> Device Serial Number could not be found in device conf");
            strcpy(gs_devSerialStr, "ZD2622HMETAL666");
            g_devSerial = gs_devSerialStr;
            DEBUG_ERROR("[E]-> Device Serial Number is default %s", g_devSerial);

            retVal = SUCCESS;
        }
    }
    else
    {
        DEBUG_WARNING("[W]-> %s does not exist, First starting !!", DEV_CONF_FILE);
    }

#ifdef __linux
    memset(confJsonBuff, 0 , sizeof(confJsonBuff));
    if (NULL == g_devSerial)
    {
        FILE* file = fopen(confPath, "rb");
        if (file)
        {
            fread(confJsonBuff, 1, sizeof(confJsonBuff), file);
            fclose(file);

            if (TRUE == ZDJson_GetStringValue(confJsonBuff, "avioraSerialNum", gs_devSerialStr, sizeof(gs_devSerialStr)))
            {
                g_devSerial = gs_devSerialStr;

                DEBUG_WARNING("[W]-> From backend, Device Serial Number %s", g_devSerial);

                FsFile *f = fsOpenFile((char *)DEV_CONF_FILE, FS_FILE_MODE_WRITE);
                if (f != NULL)
                {
                    sprintf(confJsonBuff, "{\"avioraSerialNum\":\"%s\"}", g_devSerial);
                    fsWriteFile(f, confJsonBuff, strlen(confJsonBuff));
                    fsCloseFile(f);
                }

                retVal = SUCCESS;
            }
            else
            {
                DEBUG_ERROR("[E]-> Device Serial Number could not be found in backend");
                strcpy(gs_devSerialStr, "ZD2622HMETAL666");
                g_devSerial = gs_devSerialStr;
                DEBUG_ERROR("[E]-> Device Serial Number is default %s", g_devSerial);

                retVal = SUCCESS;
            }
        }
    }

#endif

    return retVal;
}
/******************************** End Of File *********************************/
