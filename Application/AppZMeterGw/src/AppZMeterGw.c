/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 22 Mar 2024 - 11:53:31
* #File Name    : App_ZMeterGw.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppZMeterGw.h"

#include "core/net.h"

#include "AppDeviceManager.h"
#include "AppTimeService.h"
#include "AppGlobalVariables.h"
#include "AppLogRecorder.h"
#include "AppDataBus.h"
#include "AppConfigManager.h"
#include "AppDisplayManager.h"
#include "AppInternalMsgFrame.h"
#include "AppTaskManager.h"
#include "AppGsmManager.h"

#include "MiddDigitalIOControl.h"
#include "MiddStorage.h" //TODO: it could be moved to file system manager
#include "MiddEventTimer.h"

#include "Midd_OSPort.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/
static S32 gs_zmgDbusID;
static S32 gs_devMsgSN;
static OsTaskId gs_zmgTaskID;
union
{
    U32 events;
    struct
    {
        U32 inAC    : 2;  /** 1 AC off, 2 AC on, */
        U32 inSCap  : 2;  /** when SCap is full, it will be 1 */
        U32 inEA_1  : 1;
        U32 inEA_2  : 1;
        U32 inCover : 1;
        U32 inBody  : 1;
        U32 inDI_1  : 1;
        U32 inDI_2  : 1;
        U32 inDI_3  : 1;
        U32 inDI_4  : 1;
        U32 inDI_5  : 1;
        U32 inDI_6  : 1;
    };
}g_localEvents;

struct DevVar
{
    BOOL AC;    /** 1:AC input exists, 0: doesn't */
    BOOL SCAP;  /** 1:SCAP is full */
    DEV_WORKING_MODE wmode;
}x;

static DevInfoMsg gs_devVar;

/***************************** STATIC FUNCTIONS  ******************************/
static void inputACIn(U32 stat) /** AC input */
{
    g_localEvents.inAC = TRUE;
}

static void inputSCap(U32 stat) /** SCap Charge Status input, it is full */
{
    g_localEvents.inSCap = TRUE;
}

static void inputEAIn_1(U32 stat) /** Energy analyzer 1 input */
{
    g_localEvents.inEA_1 = TRUE;
}

static void inputEAIn_2(U32 stat) /** Energy analyzer 2 input */
{
    g_localEvents.inEA_2 = TRUE;
}

static void inputCover(U32 stat) /** Cover button input */
{
    g_localEvents.inCover = TRUE;
}

static void inputBody(U32 stat) /** Body button input */
{
    g_localEvents.inBody = TRUE;
}

static void inputDigital_1(U32 stat) /** Digital input 1*/
{
    g_localEvents.inDI_1 = TRUE;
}

static void inputDigital_2(U32 stat) /** Digital input 2*/
{
    g_localEvents.inDI_2 = TRUE;
}

static void inputDigital_3(U32 stat) /** Digital input 3*/
{
    g_localEvents.inDI_3 = TRUE;
}

static void inputDigital_4(U32 stat) /** Digital input 4*/
{
    g_localEvents.inDI_4 = TRUE;
}

static void inputDigital_5(U32 stat) /** Digital input 5*/
{
    g_localEvents.inDI_5 = TRUE;
}

static void inputDigital_6(U32 stat) /** Digital input 6*/
{
    g_localEvents.inDI_6 = TRUE;
}

int myWriteLog (const char *logStr)
{
    return printf("sysLog: %s\n", logStr);
}
int myReadLog (const char *logStr, int size)
{
    return 0;
}

static RETURN_STATUS initSWUnit(void)
{
    RETURN_STATUS retVal = SUCCESS;
    LogRecInterface sysLoggerInterface; //TODO: change this structure with file system w/r operation
    NtpServerConf ntpConf;

    sysLoggerInterface.writeFunc = myWriteLog;
    sysLoggerInterface.readFunc  = myReadLog;
    sysLoggerInterface.fileSize  = 100;
    sysLoggerInterface.logPath   = "./";
    sysLoggerInterface.totalLogSize = 1000;

    /** !< Firstly initialize common used midd layer */
    retVal |= middIOInit();
    retVal |= middStorageInit(); //TODO: it could be moved to file system manager
//    retVal |= middSerialCommInit();
    if (SUCCESS == retVal)
    {
        /* initialize configurations after file system */
        if (FAILURE == appConfInit("***"))
        {
            DEBUG_ERROR("->[E] Display init ERROR");
            return FAILURE;
        }

        if (SUCCESS != middEventTimerInit()) /** After OS init, call middEventTimerInit, it uses OS timer */
        {
            DEBUG_ERROR("->[E] middEventTimer init Error");
            return FAILURE;
        }

        if (FAILURE == appGlobalVarInit())
        {
            DEBUG_ERROR("->[E] appGlobalVarInit init Error");
            return FAILURE;
        }

        //TODO: init file system before log recorder
        zosInitKernel();
        zosStartKernel();

        if (FAILURE == appTskMngInit())
        {
            DEBUG_ERROR("->[E] appTskMngInit init Error");
            return FAILURE;
        }

        /* initialize log recorder after configurations */
        if (FAILURE == appLogRecInit()) /** if log register returns FAILURE, system can continue to run. */
        {
            DEBUG_ERROR("->[E] Log Recorder Init ERROR ");
            return FAILURE;
        }

        if (FAILURE == appLogRecRegister(&sysLoggerInterface, "sysLogger", &g_sysLoggerID))
        {
            DEBUG_ERROR("->[E] Log Reg for sysLogger ERROR ");
            return FAILURE;
        }

        if (FAILURE == appDBusInit())
        {
            DEBUG_ERROR("->[E] appDBusInit ERROR ");
            return FAILURE;
        }

        if (FAILURE == appGsmMngInit())
        {
            DEBUG_ERROR("->[E] appGsmMngInit ERROR ");
            return FAILURE;
        }
        if (FAILURE == appGsmMngOpenPPP())
        {
            DEBUG_ERROR("->[E] appGsmMngOpenPPP ERROR ");
            return FAILURE;
        }

        /* initialize time service after log recorder */
        if (FAILURE == appTimeServiceInit(&ntpConf))
        {
            DEBUG_ERROR("->[E] TimeSrv init Error");
            appLogRec(g_sysLoggerID, "TimeSrv init Error");
            return FAILURE;
        }

        /* initialize display after time service */
        if (FAILURE == appDisplayInit())
        {
            DEBUG_ERROR("->[E] Display init ERROR");
            appLogRec(g_sysLoggerID, "Display init Error");
            return FAILURE;
        }
        appDisplayStart();
    }

    DEBUG_INFO("->[I] initSwUnits return %d", retVal);
    return retVal;
}

static void zmgTask(void * pvParameters)
{
    DBUS_PACKET dbPacket;

    appTskMngImOK(gs_zmgTaskID);
    zosDelayTask(1000); //wait once before starting

    IpAddr ip;
    ip.length = sizeof(Ipv4Addr);
    ip.ipv4Addr  = IPV4_ADDR(88,255,75,10);
    error_t err = ERROR_FAILURE;
    char buff[16] = "";

    ipv4AddrToString(ip.ipv4Addr, buff);
    int c = 0;
    while(1)
    {
        err = ping(&netInterface[0], &ip, 32, 0xFF, 2000000, NULL);
        if(!err)
            printf("%d- Ping send to %s \n",c++, buff);
        else
            printf("Ping Error - %d !! \n", err);


        if (FALSE != g_localEvents.events)
        {
            if (TRUE == g_localEvents.inAC)
            {
                g_localEvents.inAC = FALSE; //clear the event
                if (TRUE == middIORead(EN_IN_AC_INPUT)) //AC input exist
                {
                    gs_devVar.AC = TRUE;
                    appLogRec(g_sysLoggerID, "ZMG: AC input ON");
                    DEBUG_INFO("->[I] ZMG:AC input ON");
                    /*todo: set timer to wait for super cap. If it will not be available at end of this timer,
                    system can be started without super cap */
                }
                else
                {
                    gs_devVar.AC = FALSE;

                    gs_devVar.wMode = EN_WORKING_MODE_POWER_DOWN;
                    appLogRec(g_sysLoggerID, "ZMG: AC input OFF");
                    DEBUG_INFO("->[I] ZMG:AC input OFF, POWER DOWN Mode");

                    dbPacket.packetID   = gs_devMsgSN++;
                    dbPacket.topic      = EN_DBUS_TOPIC_DEVICE;
                    dbPacket.pri        = EN_PRIORITY_HIG;
                    dbPacket.retainFlag = TRUE;

                    appIntMsgCreateDevMsg(&gs_devVar, dbPacket.payload.data, &dbPacket.payload.dataLeng);
                    appDBusPublish(gs_zmgDbusID, &dbPacket);
                }
            }

            if (TRUE == g_localEvents.inSCap)
            {
                g_localEvents.inSCap = FALSE; //clear the event
                gs_devVar.SCAP = TRUE;

                if (gs_devVar.AC)
                {
                    appLogRec(g_sysLoggerID, "ZMG:SCap READY, main mode");
                    DEBUG_INFO("->[I] ZMG:SCap READY, Main Mode");

                    gs_devVar.wMode = EN_WORKING_MODE_MAIN; /*Scap and AC input is ready now, device can switch main working mode */

                    dbPacket.packetID   = gs_devMsgSN++;
                    dbPacket.topic      = EN_DBUS_TOPIC_DEVICE;
                    dbPacket.pri        = EN_PRIORITY_HIG;
                    dbPacket.retainFlag = TRUE;

                    appIntMsgCreateDevMsg(&gs_devVar, dbPacket.payload.data, &dbPacket.payload.dataLeng);
                    appDBusPublish(gs_zmgDbusID, &dbPacket);
                }
            }
        }

        appTskMngImOK(gs_zmgTaskID);
        zosDelayTask(1000);
    }
}

/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appZMGwInit(void)
{
    RETURN_STATUS retVal;
    ZOsTaskParameters tempParam;

    tempParam.priority  = ZOS_TASK_PRIORITY_LOW;
    tempParam.stackSize = ZOS_MIN_STACK_SIZE;

    g_localEvents.events = FALSE; //clear all local events

    retVal = appDevMngInitHwUnits();

    if (SUCCESS == retVal)
    {
        retVal = initSWUnit();
    }

    if (SUCCESS == retVal)
    {
        retVal = appDBusRegister(EN_DBUS_TOPIC_NO, &gs_zmgDbusID); //EN_DBUS_TOPIC_NO, This module just publishes data
    }

    if (SUCCESS == retVal)
    {
        middIOIntListen(EN_IN_COVER_ALERT, inputCover);
        middIOIntListen(EN_IN_BODY_ALERT,  inputBody);

        middIOIntListen(EN_IN_AC_INPUT,   inputACIn);
        middIOIntListen(EN_IN_SCAP_STAT,  inputSCap);

        middIOIntListen(EN_IN_EA_INPUT_1, inputEAIn_1);
        middIOIntListen(EN_IN_EA_INPUT_2, inputEAIn_2);

        middIOIntListen(EN_IN_DI_1, inputDigital_1);
        middIOIntListen(EN_IN_DI_2, inputDigital_2);
        middIOIntListen(EN_IN_DI_3, inputDigital_3);
        middIOIntListen(EN_IN_DI_4, inputDigital_4);
        middIOIntListen(EN_IN_DI_5, inputDigital_5);
        middIOIntListen(EN_IN_DI_6, inputDigital_6);

        gs_zmgTaskID = appTskMngCreate("ZMG_TASK", zmgTask, NULL, &tempParam, TRUE);
        if (OS_INVALID_TASK_ID != gs_zmgTaskID)
        {
            zosSuspendTask(gs_zmgTaskID);
        }
        else
        {
            retVal = FAILURE;
        }
    }

    if (SUCCESS != retVal)
    {
        appDevMngHwRestart();
    }

    return retVal;
}

RETURN_STATUS appZMGwStart(void)
{
    //TODO: start all task here
    zosResumeTask(gs_zmgTaskID);

    return SUCCESS;
}
/******************************** End Of File *********************************/
