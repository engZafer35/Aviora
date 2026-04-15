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
#include "AppGlobalVariables.h"
#include "AppLogRecorder.h"

/****************************** MACRO DEFINITIONS *****************************/

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
    /* display service does not have any critical dependency, so we can start display task without waiting for any event.
     * If there will be a dependency in future, we can use the below code to wait for the dependencies.
     * For now, it is commented to avoid unnecessary waiting.
     */
    /*
    if (-1 == zosEventGroupWait(gp_systemSetupEventGrp, DISPLAY_START_DEPENDENCY_FLAGS, INFINITE_DELAY, ZOS_EVENT_WAIT_ALL))
    {
        DEBUG_ERROR("->[E] Display Task: Wait for zosEventGroupWait failed");
        APP_LOG_REC(g_sysLoggerID, "Display Task: Wait for zosEventGroupWait failed");
        appTskMngDelete(&gs_dpTaskID);        
    }
    */

    //someone may need to know display task is ready, so display ready flag is set here
    zosEventGroupSet(gp_systemSetupEventGrp, DISPLAY_SERVICE_READY_FLAG);
    DEBUG_INFO("->[I] Display Service Task started");

    while (1)
    {
        gs_currWind(); //run current window function

        appTskMngImOK(gs_dpTaskID);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appDisplayInit(void)
{
    RETURN_STATUS retVal = FAILURE;
    gs_currWind = startingWind;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    gs_dpTaskID = appTskMngCreate("DISPLAY_TASK", displayTask, NULL, &tempParam);

    if (OS_INVALID_TASK_ID != gs_dpTaskID)
    {        
        retVal = appDBusRegister(EN_DBUS_TOPIC_GSM | EN_DBUS_TOPIC_ETH | EN_DBUS_TOPIC_DEVICE, &gs_dbusID);
    }
    else
    {
        DEBUG_ERROR("->[E] Display Task could not be created");
        APP_LOG_REC(g_sysLoggerID, "Display: Task could not be created");
    }

    return retVal;
}

void appDisplayStart(void)
{
    zosResumeTask(gs_dpTaskID);
}

void appDisplayStop(void)
{
    zosSuspendTask(gs_dpTaskID);
}

/******************************** End Of File *********************************/
