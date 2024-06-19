/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Apr 17, 2024 - 10:33:23 PM
* #File Name    : AppTaskManager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppTaskManager.h"
#include "AppGlobalVariables.h"
#include "AppLogRecorder.h"

#include "Midd_OSPort.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

static OsTaskId gs_tmTaskID;

static struct
{
    OsTaskId tid;
    BOOL isAlive;
}gs_treads[MANAGE_MAX_TASK_NUMBER];
/***************************** STATIC FUNCTIONS  ******************************/

static void feedWDT(void)
{

}

static void tmTask(void * param)
{
    U32 i;

    while(1)
    {
        feedWDT();
        osDelayTask(1000);

        for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
        {
            //find relevant task and check if it is alive
            if (OS_INVALID_TASK_ID != gs_treads[i].tid)
            {
                if (TRUE == gs_treads[i].isAlive)
                {
                    //task is alive, clear data, it will be updated again by the relevant task
                    gs_treads[i].isAlive = FALSE;
                }
                else
                {
                    //upps, the thread is died, unfortunately we have to restart,
                    //TODO: call file system flush to write all pending data.
                    OsTaskInfo tinfo;

                    zosGetTaskInfo(gs_treads[i].tid, &tinfo);
                    DEBUG_ERROR("->[I] TMNG:Taskid %d not alive, Device will be restarted", gs_treads[i].tid);

#ifdef USE_FREERTOS
                    char tempBuff[256];
                    sprintf(tempBuff, "Dead Task: name: %s - CurrState: %d - Total RunTime: %d - Free stack %d ", tinfo.pcTaskName, tinfo.eCurrentState, \
                                                                                                                  tinfo.ulRunTimeCounter, tinfo.usStackHighWaterMark )
                    appLogRec(g_sysLoggerID, tempBuff);
                    DEBUG_ERROR("->[I] TMNG:Dead Task: name: %s - CurrState: %d - Total RunTime: %d - Free stack %d ", tinfo.pcTaskName, tinfo.eCurrentState, \
                                                                                                                       tinfo.ulRunTimeCounter, tinfo.usStackHighWaterMark) ;

#else
                    char tempBuff[256];
                    sprintf(tempBuff, "Dead Task: %ld", tinfo);
                    appLogRec(g_sysLoggerID, tempBuff);
#endif
                    appLogRec(g_sysLoggerID, "TMNG:Device will be restarted");

                    zosSuspendTask(gs_tmTaskID); // stop this task to restart (hw reset).
                }
            }
        }

        feedWDT();
        osDelayTask(5000);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appTskMngInit(void)
{
    RETURN_STATUS retVal = SUCCESS;
    U32 i;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_HIGH;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    //clear tread list
    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        gs_treads[i].tid       = OS_INVALID_TASK_ID;
        gs_treads[i].isAlive   = FALSE;
    }

    gs_tmTaskID = appTskMngCreate("ZMG_TASK", tmTask, NULL, &tempParam, FALSE);
    if (OS_INVALID_TASK_ID != gs_tmTaskID)
    {
        zosSuspendTask(gs_tmTaskID);
    }
    else
    {
        retVal = FAILURE;
    }

    return retVal;
}

void appTskMngStart(void)
{
    zosResumeTask(gs_tmTaskID);
}

OsTaskId appTskMngCreate(const char *name, OsTaskCode taskCode, void *arg, const ZOsTaskParameters *params, BOOL restart)
{
    OsTaskId tid;
    U32 i;

    tid = zosCreateTask(name, taskCode, NULL, params);

    if (TRUE == restart)
    {
        if (OS_INVALID_TASK_ID != tid)
        {
            for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
            {
                //find empty place and save
                if (OS_INVALID_TASK_ID != tid)
                {
                    gs_treads[i].tid     = tid;
                    gs_treads[i].isAlive = TRUE;

                    break;
                }
            }
        }
    }

    return tid;
}

RETURN_STATUS appTskMngDelete(OsTaskId tid)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    osDeleteTask(tid);

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task and update it
        if (gs_treads[i].tid == tid)
        {
            gs_treads[i].tid       = OS_INVALID_TASK_ID;
            gs_treads[i].isAlive   = FALSE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appTskMngSuspend(OsTaskId tid)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    zosSuspendTask(tid);

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task and update it
        if (gs_treads[i].tid == tid)
        {
            gs_treads[i].isAlive   = FALSE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appTskMngSuspendAll(void)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    osSuspendAllTasks();

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task, and cancel to follow the task
        if (OS_INVALID_TASK_ID != gs_treads[i].tid)
        {
            gs_treads[i].isAlive = FALSE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appTskMngResumeAll(void)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    osResumeAllTasks();

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task, and cancel to follow the task
        if (OS_INVALID_TASK_ID != gs_treads[i].tid)
        {
            gs_treads[i].isAlive = FALSE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appTskMngResume(OsTaskId tid)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    zosResumeTask(tid);

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task and update it
        if (gs_treads[i].tid == tid)
        {
            gs_treads[i].isAlive = FALSE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

RETURN_STATUS appTskMngImOK(OsTaskId tid)
{
    RETURN_STATUS retVal = FAILURE;
    U32 i;

    for (i = 0; i < MANAGE_MAX_TASK_NUMBER; i++)
    {
        //find relevant task and update it
        if (gs_treads[i].tid == tid)
        {
            gs_treads[i].isAlive = TRUE;

            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}
/******************************** End Of File *********************************/
