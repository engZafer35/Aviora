/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 27 Mar 2024 - 11:13:33
* #File Name    : AppDisplayManager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (ENABLE)
/********************************* INCLUDES ***********************************/
#include "AppDisplayManager.h"
#include "AppDataBus.h"
#include "AppTaskManager.h"


/****************************** MACRO DEFINITIONS *****************************/

/* Auto-generated display configuration - Include customer display implementation */


/******************************* TYPE DEFINITIONS *****************************/

typedef void (*WindFunc) (void);


/********************************** VARIABLES *********************************/
static WindFunc gs_currWind = NULL;
static S32 gs_dbusID;
static OsTaskId gs_dpTaskID;

/***************************** STATIC FUNCTIONS  ******************************/
#include "AppDisplayManager_Autogen.c"

static void displayTask(void * pvParameters)
{
    zosDelayTask(1000); //wait once before starting
    while (1)
    {
        gs_currWind(); //run current window function

        appTskMngImOK(gs_dpTaskID);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS AppDisplayInit(void)
{
    RETURN_STATUS retVal = FAILURE;
    gs_currWind = startingWind;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    gs_dpTaskID = appTskMngCreate("DISPLAY_TASK", displayTask, NULL, &tempParam);

    if (OS_INVALID_TASK_ID != gs_dpTaskID)
    {
        zosSuspendTask(gs_dpTaskID);
        retVal = appDBusRegister(EN_DBUS_TOPIC_GSM | EN_DBUS_TOPIC_ETH | EN_DBUS_TOPIC_DEVICE, &gs_dbusID);
    }
    else
    {
        DEBUG_ERROR("->[E] Display Task could not be created");
        APP_LOG_REC(g_sysLoggerID, "Display: Task could not be created");
    }

    return retVal;
}

void AppDisplayStart(void)
{
    zosResumeTask(gs_dpTaskID);
}

/******************************** End Of File *********************************/
