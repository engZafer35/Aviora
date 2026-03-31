/******************************************************************************
* #Author       : Zafer Satilmis
* #hype-man     : In Spite - Killing A Friend
* #Revision     : 1.0
* #Date         : Mar 31, 2026
* #File Name    : AppMqttConnService.c
*******************************************************************************/
/******************************************************************************
* MQTT broker service thread: connect/subscribe/process/reconnect using
* AppMqttConnManager (CycloneTCP). Pattern aligned with mass_conn_thread_entry.
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include "AppMqttConnService.h"
#include "AppMqttConnManager.h"

#include "AppTaskManager.h"
#include "Midd_OSPort.h"

#include <string.h>

/****************************** MACRO DEFINITIONS *****************************/
#define APP_MQTT_SVC_RET_SUCCESS    (0)
#define APP_MQTT_SVC_RET_FAILURE    (-1)

#define APP_MQTT_SVC_BROKER_IP_LEN  (16U)
#define APP_MQTT_SVC_AUTH_STR_MAX   (32U)
#define APP_MQTT_SVC_RECONNECT_MS   (2000U)
#define APP_MQTT_SVC_LOOP_MS        (100U)
#define APP_MQTT_SVC_SUB_GAP_MS     (200U)

#ifndef TRUE
#define TRUE    (1)
#endif
#ifndef FALSE
#define FALSE   (0)
#endif

/******************************* TYPE DEFINITIONS *******************************/

typedef struct AppMqttSvcTaskBits_s
{
    unsigned int taskConnect      : 1;
    unsigned int taskDisconnect   : 1;
    unsigned int taskReconnect    : 1;
    unsigned int taskSubscribe    : 1;
    unsigned int userWantOnline   : 1;
    unsigned int linkUpNotified   : 1;
    unsigned int stopRequested    : 1;
} AppMqttSvcTaskBits_t;

/********************************** VARIABLES *********************************/
static OsTaskId gs_svcTaskId = OS_INVALID_TASK_ID;

static volatile AppMqttSvcTaskBits_t gs_bits;

static char gs_brokerIp[APP_MQTT_SVC_BROKER_IP_LEN];
static char gs_userName[APP_MQTT_SVC_AUTH_STR_MAX];
static char gs_psw[APP_MQTT_SVC_AUTH_STR_MAX];
static char gs_mqttClientId[APP_MQTT_CONN_MNG_CLIENT_ID_BUF_LEN];
static int gs_brokerPort;

static char gs_topicBuf[APP_MQTT_CONN_MNG_MAX_TOPIC_NUM][APP_MQTT_CONN_MNG_MAX_TOPIC_NAME_LEN];
/** Topic string for slot `i` (empty slot: first byte is '\0'). */
#define TOPIC_NAME(i)               (gs_topicBuf[(i)])

static AppMqttConnServiceIncomingCb_t gs_incomingCb;
static AppMqttConnServiceLinkCb_t gs_linkCb;

/***************************** STATIC FUNCTIONS  ******************************/

static void appMqttSvcNotifyLink(int isUp)
{
    if (gs_linkCb != NULL)
    {
        gs_linkCb(isUp);
    }
}

static void appMqttSvcBuildMqttClientId(void)
{
    static const char prefix[] = "z_";
    size_t pi;
    size_t bi;
    const char *nick = APP_MQTT_CONN_SERVICE_BROKER_NAME;
    char *dst = gs_mqttClientId;
    const size_t dstLen = sizeof(gs_mqttClientId);

    pi = 0U;
    while ((pi < (sizeof(prefix) - 1U)) && (pi < (dstLen - 1U)))
    {
        dst[pi] = prefix[pi];
        pi++;
    }

    for (bi = 0U; (pi < (dstLen - 1U)) && (nick[bi] != '\0'); bi++)
    {
        dst[pi] = nick[bi];
        pi++;
    }

    dst[pi] = '\0';
}

static void appMqttSvcNotifyLinkDownIfWasUp(void)
{
    if (0 != gs_bits.linkUpNotified)
    {
        gs_bits.linkUpNotified = FALSE;
        appMqttSvcNotifyLink(0);
    }
}

static void appMqttSvcConnInfoCb(AppMqttConnStatus_t stat)
{
    switch (stat)
    {
        case APP_MQTT_STAT_CONNECTION_OK:
            gs_bits.taskSubscribe = TRUE;
            break;

        case APP_MQTT_STAT_CONNECTION_FAIL:
            appMqttSvcNotifyLinkDownIfWasUp();
            gs_bits.taskConnect = TRUE;
            break;

        case APP_MQTT_STAT_SUBS_OK:
            break;

        case APP_MQTT_STAT_SUBS_FAIL:
            gs_bits.taskReconnect = TRUE;
            gs_bits.taskSubscribe = FALSE;
            break;

        case APP_MQTT_STAT_UNSUBS_OK:
        case APP_MQTT_STAT_UNSUBS_FAIL:
        case APP_MQTT_STAT_PUB_OK:
        case APP_MQTT_STAT_PUB_FAIL:
        default:
            break;
    }
}

static void appMqttSvcListenCb(const char *topic, const void *rcvBuff, unsigned int buffLeng)
{
    if ((gs_incomingCb != NULL) && (topic != NULL) && (rcvBuff != NULL))
    {
        gs_incomingCb(topic, (const char *)rcvBuff, buffLeng);
    }
}

static int appMqttSvcTaskConnect(void)
{
    appMqttSvcBuildMqttClientId();

    return appMqttConnMngConnectBroker(gs_brokerIp, (unsigned short)gs_brokerPort,
                                       gs_userName, gs_psw, gs_mqttClientId,
                                       appMqttSvcConnInfoCb, appMqttSvcListenCb);
}

static int appMqttSvcTaskDisconnect(void)
{
    if (0 == appMqttConnServiceIsConnected())
    {
        appMqttSvcNotifyLinkDownIfWasUp();
        return APP_MQTT_SVC_RET_SUCCESS;
    }

    if (0 == appMqttConnMngDisconnectBroker())
    {
        appMqttSvcNotifyLinkDownIfWasUp();
        return APP_MQTT_SVC_RET_SUCCESS;
    }

    return APP_MQTT_SVC_RET_FAILURE;
}

static int appMqttSvcTaskReconnect(void)
{
    (void)appMqttSvcTaskDisconnect();
    gs_bits.taskConnect = TRUE;
    return APP_MQTT_SVC_RET_SUCCESS;
}

static int appMqttSvcTaskSubsAll(void)
{
    unsigned int i;
    int anyFail = FALSE;

    for (i = 0U; i < APP_MQTT_CONN_MNG_MAX_TOPIC_NUM; i++)
    {
        if ('\0' != TOPIC_NAME(i)[0])
        {
            if (0 != appMqttConnMngSubscribe(TOPIC_NAME(i)))
            {
                anyFail = TRUE;
            }
            zosDelayTask((systime_t)APP_MQTT_SVC_SUB_GAP_MS);
        }
    }

    return anyFail ? APP_MQTT_SVC_RET_FAILURE : APP_MQTT_SVC_RET_SUCCESS;
}

static void appMqttSvcThread(void *arg)
{
    unsigned int i;

    (void)arg;

    while (FALSE == gs_bits.stopRequested)
    {
        appTskMngImOK(gs_svcTaskId);

        if (0 != gs_bits.userWantOnline)
        {
            if (0 != gs_bits.taskConnect)
            {
                if (APP_MQTT_SVC_RET_SUCCESS == appMqttSvcTaskConnect())
                {
                    gs_bits.taskConnect = FALSE;
                }
                else
                {
                    zosDelayTask((systime_t)APP_MQTT_SVC_RECONNECT_MS);
                }
            }

            if (0 != gs_bits.taskDisconnect)
            {
                if (APP_MQTT_SVC_RET_SUCCESS == appMqttSvcTaskDisconnect())
                {
                    gs_bits.taskDisconnect = FALSE;
                    gs_bits.userWantOnline = FALSE;
                }
            }

            if (0 != gs_bits.taskReconnect)
            {
                if (APP_MQTT_SVC_RET_SUCCESS == appMqttSvcTaskReconnect())
                {
                    gs_bits.taskReconnect = FALSE;
                }
            }

            if (0 != gs_bits.taskSubscribe)
            {
                if (0 != appMqttConnServiceIsConnected())
                {
                    if (APP_MQTT_SVC_RET_SUCCESS != appMqttSvcTaskSubsAll())
                    {
                        gs_bits.taskReconnect = TRUE;
                    }
                    else if (0 == gs_bits.linkUpNotified)
                    {
                        gs_bits.linkUpNotified = TRUE;
                        appMqttSvcNotifyLink(1);
                    }
                }
                gs_bits.taskSubscribe = FALSE;
            }

            if (TRUE == appMqttConnServiceIsConnected())
            {
                if (0 != appMqttConnMngProcess())
                {
                    /* appMqttConnMngMarkLinkDown -> connInfoCb(CONNECTION_FAIL) */
                    gs_bits.taskConnect = TRUE;
                    zosDelayTask((systime_t)APP_MQTT_SVC_RECONNECT_MS);
                }
            }
        }
        else
        {
            if (TRUE == appMqttConnServiceIsConnected())
            {
                (void)appMqttSvcTaskDisconnect();
            }
        }

        zosDelayTask((systime_t)APP_MQTT_SVC_LOOP_MS);
    }

    (void)appMqttSvcTaskDisconnect();

    for (i = 0U; i < APP_MQTT_CONN_MNG_MAX_TOPIC_NUM; i++)
    {
        TOPIC_NAME(i)[0] = '\0';
    }

    gs_brokerIp[0] = '\0';
    gs_userName[0] = '\0';
    gs_psw[0] = '\0';
    gs_mqttClientId[0] = '\0';
    gs_brokerPort = 0;
    gs_incomingCb = NULL;
    gs_linkCb = NULL;
    memset((void *)&gs_bits, 0, sizeof(gs_bits));

    (void)appTskMngDelete(&gs_svcTaskId);
}

/***************************** PUBLIC FUNCTIONS  ******************************/

int appMqttConnServiceStart(const char *brokerIP, int brokerPort,
                            const char *userName, const char *psw,
                            const char *subsTopic,
                            AppMqttConnServiceIncomingCb_t incomingCb,
                            AppMqttConnServiceLinkCb_t linkCb)
{
    ZOsTaskParameters taskParam;

    if ((brokerIP == NULL) || (subsTopic == NULL) || (incomingCb == NULL))
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    if (OS_INVALID_TASK_ID != gs_svcTaskId)
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    memset((void *)&gs_bits, 0, sizeof(gs_bits));
    memset(gs_topicBuf, 0, sizeof(gs_topicBuf));

    strncpy(gs_brokerIp, brokerIP, sizeof(gs_brokerIp) - 1U);
    gs_brokerIp[sizeof(gs_brokerIp) - 1U] = '\0';

    if (userName != NULL)
    {
        strncpy(gs_userName, userName, sizeof(gs_userName) - 1U);
        gs_userName[sizeof(gs_userName) - 1U] = '\0';
    }
    else
    {
        gs_userName[0] = '\0';
    }

    if (psw != NULL)
    {
        strncpy(gs_psw, psw, sizeof(gs_psw) - 1U);
        gs_psw[sizeof(gs_psw) - 1U] = '\0';
    }
    else
    {
        gs_psw[0] = '\0';
    }

    gs_brokerPort = brokerPort;

    strncpy(TOPIC_NAME(0), subsTopic, sizeof(gs_topicBuf[0]) - 1U);
    TOPIC_NAME(0)[sizeof(gs_topicBuf[0]) - 1U] = '\0';

    gs_incomingCb = incomingCb;
    gs_linkCb = linkCb;

    taskParam.priority  = ZOS_TASK_PRIORITY_LOW;
    taskParam.stackSize = ZOS_MIN_STACK_SIZE * 2U;

    gs_bits.stopRequested = FALSE;
    gs_svcTaskId = appTskMngCreate("MqttSvc", appMqttSvcThread, NULL, &taskParam);
    if (OS_INVALID_TASK_ID == gs_svcTaskId)
    {
        gs_incomingCb = NULL;
        gs_linkCb = NULL;
        TOPIC_NAME(0)[0] = '\0';
        return APP_MQTT_SVC_RET_FAILURE;
    }

    return APP_MQTT_SVC_RET_SUCCESS;
}

int appMqttConnServiceStop(void)
{
    if (OS_INVALID_TASK_ID == gs_svcTaskId)
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    gs_bits.stopRequested = TRUE;

    return APP_MQTT_SVC_RET_SUCCESS;
}

int appMqttConnServiceIsConnected(void)
{
    return appMqttConnMngIsConnected();
}

int appMqttConnServiceRequestConnect(void)
{
    if (OS_INVALID_TASK_ID == gs_svcTaskId)
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    gs_bits.userWantOnline = TRUE;
    gs_bits.taskConnect = TRUE;

    return APP_MQTT_SVC_RET_SUCCESS;
}

int appMqttConnServiceRequestDisconnect(void)
{
    if (OS_INVALID_TASK_ID == gs_svcTaskId)
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    gs_bits.taskDisconnect = TRUE;

    return APP_MQTT_SVC_RET_SUCCESS;
}

int appMqttConnServicePublish(const char *topic, const char *data, unsigned int dataLeng)
{
    return appMqttConnMngPublish(topic, data, dataLeng);
}

int appMqttConnServiceSubscribe(const char *topic)
{
    unsigned int i;

    if ((topic == NULL) || (OS_INVALID_TASK_ID == gs_svcTaskId))
    {
        return APP_MQTT_SVC_RET_FAILURE;
    }

    for (i = 0U; i < APP_MQTT_CONN_MNG_MAX_TOPIC_NUM; i++)
    {
        if (('\0' != TOPIC_NAME(i)[0]) && (0 == strcmp(TOPIC_NAME(i), topic)))
        {
            return APP_MQTT_SVC_RET_SUCCESS;
        }
    }

    for (i = 0U; i < APP_MQTT_CONN_MNG_MAX_TOPIC_NUM; i++)
    {
        if ('\0' == TOPIC_NAME(i)[0])
        {
            strncpy(TOPIC_NAME(i), topic, sizeof(gs_topicBuf[i]) - 1U);
            TOPIC_NAME(i)[sizeof(gs_topicBuf[i]) - 1U] = '\0';

            if (0 != appMqttConnServiceIsConnected())
            {
                if (0 != appMqttConnMngSubscribe(TOPIC_NAME(i)))
                {
                    TOPIC_NAME(i)[0] = '\0';
                    return APP_MQTT_SVC_RET_FAILURE;
                }
                zosDelayTask((systime_t)APP_MQTT_SVC_SUB_GAP_MS);
            }

            return APP_MQTT_SVC_RET_SUCCESS;
        }
    }

    return APP_MQTT_SVC_RET_FAILURE;
}

/******************************** End Of File *********************************/
