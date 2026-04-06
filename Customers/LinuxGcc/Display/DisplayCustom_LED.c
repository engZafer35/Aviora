/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 12 Mar 2026 - 18:51:33
* #File Name    : DisplayCustom_LED.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
/********************************* INCLUDES ***********************************/
#include "DisplayCustom_LED.h"
#include "AppDataBus.h"
#include "AppLogRecorder.h"
#include "AppGlobalVariables.h"
#include "AppInternalMsgFrame.h"

#include "MiddDigitalIOControl.h"

/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/
typedef enum
{
    EN_DISPLAY_WINDOW_STARTING,
    EN_DISPLAY_WINDOW_MAIN,
    EN_DISPLAY_WINDOW_SW_UPDATING,
    EN_DISPLAY_WINDOW_NO_GSM_CONN,
    EN_DISPLAY_WINDOW_FAILURE,     /* If a fatal error occurs device will switch to this mode */

    EN_DISPLAY_WINDOW_MAX_NUMBER
}EN_DISPLAY_WINDOW;

typedef void (*WindFunc) (void);

typedef struct
{
    int  gsmSigLevel;
    BOOL gsmModemStat;
    BOOL connStat;

    int relayStat;

    DEV_WORKING_MODE workingMode;
}DisplayDataStr;

/********************************** VARIABLES *********************************/
static DisplayDataStr gs_dspData;

/***************************** STATIC FUNCTIONS  ******************************/
static void setCurrentWindFunc(EN_DISPLAY_WINDOW nextWind);

static void clearDisplay(void)
{
    middIOWrite(EN_OUT_GSM_LED_1, FALSE);
    middIOWrite(EN_OUT_GSM_LED_2, FALSE);
    middIOWrite(EN_OUT_GSM_LED_3, FALSE);
    middIOWrite(EN_OUT_GSM_LED_4, FALSE);
    middIOWrite(EN_OUT_GSM_LED_5, FALSE);

    middIOWrite(EN_OUT_GSM_CONN_LED, FALSE);
    middIOWrite(EN_OUT_GSM_INTERNET_LED, FALSE);

//    middIOWrite(EN_OUT_JOB_STATUS_LED, FALSE);  /** Display doesn't control these leds */
//    middIOWrite(EN_OUT_POWER_LED, FALSE);
}

static void showGsmLevel(int level)
{
    middIOWrite(EN_OUT_GSM_LED_1, FALSE);
    middIOWrite(EN_OUT_GSM_LED_2, FALSE);
    middIOWrite(EN_OUT_GSM_LED_3, FALSE);
    middIOWrite(EN_OUT_GSM_LED_4, FALSE);
    middIOWrite(EN_OUT_GSM_LED_5, FALSE);

    if (level < 21)
    {
        middIOWrite(EN_OUT_GSM_LED_1, TRUE);
    }
    else if (level < 41)
    {
        middIOWrite(EN_OUT_GSM_LED_1, TRUE);
        middIOWrite(EN_OUT_GSM_LED_2, TRUE);
    }
    else if (level < 61)
    {
        middIOWrite(EN_OUT_GSM_LED_1, TRUE);
        middIOWrite(EN_OUT_GSM_LED_2, TRUE);
        middIOWrite(EN_OUT_GSM_LED_3, TRUE);
    }
    else if (level < 81)
    {
        middIOWrite(EN_OUT_GSM_LED_1, TRUE);
        middIOWrite(EN_OUT_GSM_LED_2, TRUE);
        middIOWrite(EN_OUT_GSM_LED_3, TRUE);
        middIOWrite(EN_OUT_GSM_LED_4, TRUE);
    }
    else if (level < 101)
    {
        middIOWrite(EN_OUT_GSM_LED_1, TRUE);
        middIOWrite(EN_OUT_GSM_LED_2, TRUE);
        middIOWrite(EN_OUT_GSM_LED_3, TRUE);
        middIOWrite(EN_OUT_GSM_LED_4, TRUE);
        middIOWrite(EN_OUT_GSM_LED_5, TRUE);
    }
}

static void showGsmConnStat(BOOL level)
{
    middIOWrite(EN_OUT_GSM_CONN_LED, level);
}

static void showGsmInternetConn(BOOL level)
{
    middIOWrite(EN_OUT_GSM_INTERNET_LED, level);
}

static void startingWind(void)
{
    DBUS_PACKET busMsg;
    U32 i;
    DevInfoMsg devMsg;
    GsmMsg gsmMsg;

    APP_LOG_REC(g_sysLoggerID, "Display:Starting mode");
    DEBUG_INFO("->[I] Display:Starting mode");
    while(1)
    {
        /** show led animation during starting mode*/
        for (i = 0; i < EN_OUT_MAX_NUMBER; i++)
        {
            middIOCtrlToggleLed(i);
            zosDelayTask(250);
        }

        appTskMngImOK(gs_dpTaskID);

        while(1)
        {
            if (SUCCESS == appDBusReceive(gs_dbusID, &busMsg, WAIT_10_MS))
            {
                DEBUG_INFO("->[I] Display: Dbus raw msg pckID: %d, topic: %d ", busMsg.packetID, busMsg.topic);
                if (busMsg.topic & EN_DBUS_TOPIC_DEVICE)
                {
                    if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                    {
                        if (gs_dspData.workingMode != devMsg.wMode)
                        {
                            DEBUG_INFO("->[I] Display: Dbus DevMsg w.Mode: %d", devMsg.wMode);
                            gs_dspData.workingMode = devMsg.wMode;
                            if (EN_WORKING_MODE_FAILURE == gs_dspData.workingMode)                { setCurrentWindFunc(EN_DISPLAY_WINDOW_FAILURE); }
                            else if (EN_WORKING_MODE_NO_NETWORK == gs_dspData.workingMode) { setCurrentWindFunc(EN_DISPLAY_WINDOW_NO_GSM_CONN); }
                            else if (EN_WORKING_MODE_SW_UPDATING == gs_dspData.workingMode)       { setCurrentWindFunc(EN_DISPLAY_WINDOW_SW_UPDATING); }
                            else if (EN_WORKING_MODE_MAIN == gs_dspData.workingMode)              { setCurrentWindFunc(EN_DISPLAY_WINDOW_MAIN); }
                            /* else if (EN_WORKING_MODE_POWER_DOWN == gs_dspData.workingMode)  !< not need to handle power down mode. keep to working as is */
                            break;
                        }
                    }
                }
                else if (busMsg.topic & EN_DBUS_TOPIC_GSM)
                {
                    if (SUCCESS == appIntMsgParseGsmMsg(busMsg.payload.data, busMsg.payload.dataLeng, &gsmMsg))
                    {
                        gs_dspData.gsmSigLevel  = gsmMsg.signalLevel;
                        gs_dspData.gsmModemStat = gsmMsg.modemStat;
                        gs_dspData.connStat     = gsmMsg.connStat;
                    }
                }
            }
            else
            {
                break; /** DBus is empty */
            }

            appTskMngImOK(gs_dpTaskID);
        }

        if (startingWind != gs_currWind)
        {
            break;
        }
    }
}

static void mainWind(void)
{
    DBUS_PACKET busMsg;
    DevInfoMsg devMsg;
    GsmMsg gsmMsg;

    DEBUG_INFO("->[I] Display:Main Window Started");
    APP_LOG_REC(g_sysLoggerID, "Display:Main Window");

    clearDisplay();
    showGsmLevel(gs_dspData.gsmSigLevel);
    showGsmConnStat(gs_dspData.gsmModemStat);
    showGsmInternetConn(gs_dspData.connStat);

    while(1)
    {
        if (SUCCESS == appDBusReceive(gs_dbusID, &busMsg, WAIT_10_MS))
        {
            if (busMsg.topic & EN_DBUS_TOPIC_DEVICE)
            {
                if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                {
                    if (gs_dspData.workingMode != devMsg.wMode)
                    {
                        gs_dspData.workingMode = devMsg.wMode;
                        if (EN_WORKING_MODE_FAILURE == gs_dspData.workingMode)                { setCurrentWindFunc(EN_DISPLAY_WINDOW_FAILURE); }
                        else if (EN_WORKING_MODE_NO_NETWORK == gs_dspData.workingMode)        { setCurrentWindFunc(EN_DISPLAY_WINDOW_NO_GSM_CONN); }
                        else if (EN_WORKING_MODE_SW_UPDATING == gs_dspData.workingMode)       { setCurrentWindFunc(EN_DISPLAY_WINDOW_SW_UPDATING); }
                        else if (EN_WORKING_MODE_STARTING == gs_dspData.workingMode)          { setCurrentWindFunc(EN_DISPLAY_WINDOW_STARTING); }
                        /* else if (EN_WORKING_MODE_POWER_DOWN == gs_dspData.workingMode)  !< not need to handle power down mode. keep to working as is */
                    }
                }
            }
            else if (busMsg.topic & EN_DBUS_TOPIC_GSM)
            {
                appIntMsgParseGsmMsg(busMsg.payload.data, busMsg.payload.dataLeng, &gsmMsg);

                gs_dspData.gsmSigLevel = gsmMsg.signalLevel;
                showGsmLevel(gs_dspData.gsmSigLevel);

                gs_dspData.gsmModemStat = gsmMsg.modemStat;
                showGsmConnStat(gs_dspData.gsmModemStat);

                gs_dspData.connStat = gsmMsg.connStat;
                showGsmInternetConn(gs_dspData.connStat);
            }
        }

        /**
         *  Note: when device switch to power down mode, display doesn't need to change window, stay in main window
         *  That is, there is not a special window for power down mode
         */

        zosDelayTask(1000); //refresh the screen in 1 min

        if (mainWind != gs_currWind)
        {
            break;
        }
    }
}

static void swUpdatingWind(void)
{
    DBUS_PACKET busMsg;
    DevInfoMsg devMsg;

    DEBUG_INFO("->[I] Display:SW Updating Window");
    APP_LOG_REC(g_sysLoggerID, "Display:SW Updating Window");

    clearDisplay();

    while(1)
    {
        if (SUCCESS == appDBusReceive(gs_dbusID, &busMsg, WAIT_10_MS))
        {
            if (busMsg.topic & EN_DBUS_TOPIC_DEVICE) //just handle device message
            {
                if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                {
                    if (gs_dspData.workingMode != devMsg.wMode)
                    {
                        gs_dspData.workingMode = devMsg.wMode;
                        if (EN_WORKING_MODE_FAILURE == gs_dspData.workingMode)                { setCurrentWindFunc(EN_DISPLAY_WINDOW_FAILURE); }
                        else if (EN_WORKING_MODE_NO_NETWORK == gs_dspData.workingMode) { setCurrentWindFunc(EN_DISPLAY_WINDOW_NO_GSM_CONN);}
                        else if (EN_WORKING_MODE_MAIN == gs_dspData.workingMode)              { setCurrentWindFunc(EN_DISPLAY_WINDOW_MAIN); }
                        else if (EN_WORKING_MODE_STARTING == gs_dspData.workingMode)          { setCurrentWindFunc(EN_DISPLAY_WINDOW_STARTING);}
                        /* else if (EN_WORKING_MODE_POWER_DOWN == gs_dspData.workingMode)  !< not need to handle power down mode. keep to working as is */
                    }
                }
            }
        }

        /** Toggle gsm level led during sw update period */
        middIOCtrlToggleLed(EN_OUT_GSM_LED_1);
        middIOCtrlToggleLed(EN_OUT_GSM_LED_2);
        middIOCtrlToggleLed(EN_OUT_GSM_LED_3);
        middIOCtrlToggleLed(EN_OUT_GSM_LED_4);
        middIOCtrlToggleLed(EN_OUT_GSM_LED_5);

        zosDelayTask(1000); //refresh the screen in 1 min

        if (swUpdatingWind != gs_currWind)
        {
            break;
        }
    }
}

static void noGsmConnWind(void)
{
    DBUS_PACKET busMsg;
    DevInfoMsg devMsg;

    DEBUG_INFO("->[I] Display: No Connection Window");
    APP_LOG_REC(g_sysLoggerID, "Display:No Connection Window");

    clearDisplay();

    while(1)
    {
        if (SUCCESS == appDBusReceive(gs_dbusID, &busMsg, WAIT_10_MS))
        {
            if (busMsg.topic & EN_DBUS_TOPIC_DEVICE) //just handle device message
            {
                if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                {
                    if (gs_dspData.workingMode != devMsg.wMode)
                    {
                        gs_dspData.workingMode = devMsg.wMode;
                        if (EN_WORKING_MODE_FAILURE == gs_dspData.workingMode)                { setCurrentWindFunc(EN_DISPLAY_WINDOW_FAILURE); }
                        else if (EN_WORKING_MODE_NO_NETWORK == gs_dspData.workingMode)        { setCurrentWindFunc(EN_DISPLAY_WINDOW_NO_GSM_CONN); }
                        else if (EN_WORKING_MODE_SW_UPDATING == gs_dspData.workingMode)       { setCurrentWindFunc(EN_DISPLAY_WINDOW_SW_UPDATING); }
                        else if (EN_WORKING_MODE_MAIN == gs_dspData.workingMode)              { setCurrentWindFunc(EN_DISPLAY_WINDOW_MAIN); }
                        /* else if (EN_WORKING_MODE_POWER_DOWN == gs_dspData.workingMode)  !< not need to handle power down mode. keep to working as is */
                        break;
                    }
                }
            }
        }

        /** Toggle leds in no connection mode */
        middIOCtrlToggleLed(EN_OUT_GSM_CONN_LED);
        middIOCtrlToggleLed(EN_OUT_GSM_INTERNET_LED);

        zosDelayTask(100); //refresh the screen in 1 sec

        if (noGsmConnWind != gs_currWind)
        {
            break;
        }
    }
}

static void failureWind(void)
{
    DBUS_PACKET busMsg;
    DevInfoMsg devMsg;

    DEBUG_INFO("->[I] Display:SW Updating Window");
    APP_LOG_REC(g_sysLoggerID, "Display:SW Updating Window");

    clearDisplay();

    while(1)
    {
        if (SUCCESS == appDBusReceive(gs_dbusID, &busMsg, WAIT_10_MS))
        {
            if (busMsg.topic & EN_DBUS_TOPIC_DEVICE) //just handle device message
            {
                if (SUCCESS == appIntMsgParseDevMsg(busMsg.payload.data, busMsg.payload.dataLeng, &devMsg))
                {
                    if (gs_dspData.workingMode != devMsg.wMode)
                    {
                        gs_dspData.workingMode = devMsg.wMode;
                        if (EN_WORKING_MODE_STARTING == gs_dspData.workingMode)               { setCurrentWindFunc(EN_DISPLAY_WINDOW_STARTING); }
                        else if (EN_WORKING_MODE_NO_NETWORK == gs_dspData.workingMode)        { setCurrentWindFunc(EN_DISPLAY_WINDOW_NO_GSM_CONN); }
                        else if (EN_WORKING_MODE_SW_UPDATING == gs_dspData.workingMode)       { setCurrentWindFunc(EN_DISPLAY_WINDOW_SW_UPDATING); }
                        else if (EN_WORKING_MODE_MAIN == gs_dspData.workingMode)              { setCurrentWindFunc(EN_DISPLAY_WINDOW_MAIN); }
                        /* else if (EN_WORKING_MODE_POWER_DOWN == gs_dspData.workingMode)  !< not need to handle power down mode. keep to working as is */
                        break;
                    }
                }
            }
        }

        /** Toggle leds in failure mode */
        middIOCtrlToggleLed(EN_OUT_POWER_LED);
        middIOCtrlToggleLed(EN_OUT_GSM_CONN_LED);
        middIOCtrlToggleLed(EN_OUT_GSM_INTERNET_LED);

        zosDelayTask(1000); //refresh the screen in 1 min

        if (failureWind != gs_currWind)
        {
            break;
        }
    }
}

static void setCurrentWindFunc(EN_DISPLAY_WINDOW nextWind)
{
    switch(nextWind)
    {
        case EN_DISPLAY_WINDOW_STARTING:    gs_currWind = startingWind;    break;
        case EN_DISPLAY_WINDOW_MAIN:        gs_currWind = mainWind;        break;
        case EN_DISPLAY_WINDOW_SW_UPDATING: gs_currWind = swUpdatingWind;  break;
        case EN_DISPLAY_WINDOW_NO_GSM_CONN: gs_currWind = noGsmConnWind;   break;
        case EN_DISPLAY_WINDOW_FAILURE:     gs_currWind = failureWind;     break;
    }
}

/****************************** END OF FILE *****************************/
