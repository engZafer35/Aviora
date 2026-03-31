/******************************************************************************
* #Author       : Zafer Satilmis
* #hype-man     : In Spite - The Miserables
* #Revision     : 2.0
* #Date         : Mar 31, 2026
* #File Name    : AppMqttConnManager.c
*******************************************************************************/
/******************************************************************************
* MQTT transport (CycloneTCP mqtt_client). Policy and strings (broker id, topics)
* belong to the service layer.
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include "AppMqttConnManager.h"

#include "core/net.h"
#include "core/ip.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_common.h"

#include <string.h>

/****************************** MACRO DEFINITIONS *****************************/
#define APP_MQTT_CONN_MNG_RET_SUCCESS    (0)
#define APP_MQTT_CONN_MNG_RET_FAILURE    (-1)

#ifndef NULL
#define NULL    ((void *)0)
#endif

#define APP_MQTT_CONN_MNG_QOS    (MQTT_QOS_LEVEL_0)

/******************************* TYPE DEFINITIONS *******************************/

typedef struct AppMqttConnMngCtx_s
{
    int isConnected;
    AppMqttConnMngConnInfoCb_t connCb;
    AppMqttConnMngListenCb_t publishCb;
    MqttClientContext mqtt;
} AppMqttConnMngCtx_t;

/********************************** VARIABLES *********************************/
static AppMqttConnMngCtx_t gs_ctx;

/***************************** STATIC FUNCTIONS  ******************************/

static void appMqttConnMngNotify(AppMqttConnStatus_t stat)
{
    if (gs_ctx.connCb != NULL)
    {
        gs_ctx.connCb(stat);
    }
}

static void appMqttConnMngPublishDispatch(MqttClientContext *context,
    const char_t *topic, const uint8_t *message, size_t length,
    bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId)
{
    (void)context;
    (void)dup;
    (void)qos;
    (void)retain;
    (void)packetId;

    if ((topic == NULL) || (message == NULL))
    {
        return;
    }

    if (gs_ctx.publishCb != NULL)
    {
        gs_ctx.publishCb(topic, message, (unsigned int)length);
    }
}

static void appMqttConnMngMarkLinkDown(void)
{
    if (0 == gs_ctx.isConnected)
    {
        return;
    }

    gs_ctx.isConnected = 0;
    gs_ctx.publishCb = NULL;
    mqttClientDeinit(&gs_ctx.mqtt);
    appMqttConnMngNotify(APP_MQTT_STAT_CONNECTION_FAIL);
}

static int appMqttConnMngApplyAuth(const char *clientName, const char *clientPsw)
{
    error_t e;
    const char *user;
    const char *pass;

    user = ((clientName != NULL) && (clientName[0] != '\0')) ? clientName : "";
    pass = ((clientPsw != NULL) && (clientPsw[0] != '\0')) ? clientPsw : "";

    if ((user[0] == '\0') && (pass[0] == '\0'))
    {
        return APP_MQTT_CONN_MNG_RET_SUCCESS;
    }

    e = mqttClientSetAuthInfo(&gs_ctx.mqtt, user, pass);
    if (e != NO_ERROR)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

/***************************** PUBLIC FUNCTIONS  ******************************/

int appMqttConnMngConnectBroker(const char *ip, unsigned short port,
                                const char *clientName, const char *clientPsw,
                                const char *clientId,
                                AppMqttConnMngConnInfoCb_t connCb,
                                AppMqttConnMngListenCb_t publishCb)
{
    error_t err;
    IpAddr serverIp;

    if ((ip == NULL) || (clientId == NULL) || (connCb == NULL) || (publishCb == NULL))
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 != gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = ipStringToAddr(ip, &serverIp);
    if (err != NO_ERROR)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientInit(&gs_ctx.mqtt);
    if (err != NO_ERROR)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientBindToInterface(&gs_ctx.mqtt, netGetDefaultInterface());
    if (err != NO_ERROR)
    {
        mqttClientDeinit(&gs_ctx.mqtt);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    (void)mqttClientSetTimeout(&gs_ctx.mqtt, (systime_t)MQTT_CLIENT_DEFAULT_TIMEOUT);
    (void)mqttClientSetKeepAlive(&gs_ctx.mqtt, 60U);

    err = mqttClientSetIdentifier(&gs_ctx.mqtt, clientId);
    if (err != NO_ERROR)
    {
        mqttClientDeinit(&gs_ctx.mqtt);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (APP_MQTT_CONN_MNG_RET_SUCCESS != appMqttConnMngApplyAuth(clientName, clientPsw))
    {
        mqttClientDeinit(&gs_ctx.mqtt);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientRegisterPublishCallback(&gs_ctx.mqtt, appMqttConnMngPublishDispatch);
    if (err != NO_ERROR)
    {
        mqttClientDeinit(&gs_ctx.mqtt);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb = connCb;
    gs_ctx.publishCb = publishCb;

    err = mqttClientConnect(&gs_ctx.mqtt, &serverIp, port, TRUE);
    if (err != NO_ERROR)
    {
        gs_ctx.connCb = NULL;
        gs_ctx.publishCb = NULL;
        mqttClientDeinit(&gs_ctx.mqtt);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.isConnected = 1;

    appMqttConnMngNotify(APP_MQTT_STAT_CONNECTION_OK);

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngDisconnectBroker(void)
{
    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb = NULL;
    gs_ctx.publishCb = NULL;

    (void)mqttClientDisconnect(&gs_ctx.mqtt);
    mqttClientDeinit(&gs_ctx.mqtt);

    gs_ctx.isConnected = 0;

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngSubscribe(const char *topic)
{
    error_t err;

    if (topic == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientSubscribe(&gs_ctx.mqtt, topic, APP_MQTT_CONN_MNG_QOS, NULL);
    if (err != NO_ERROR)
    {
        appMqttConnMngNotify(APP_MQTT_STAT_SUBS_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    appMqttConnMngNotify(APP_MQTT_STAT_SUBS_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngUnsubscribe(const char *topic)
{
    error_t err;

    if (topic == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientUnsubscribe(&gs_ctx.mqtt, topic, NULL);
    if (err != NO_ERROR)
    {
        appMqttConnMngNotify(APP_MQTT_STAT_UNSUBS_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    appMqttConnMngNotify(APP_MQTT_STAT_UNSUBS_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngPublish(const char *topic, const char *buff, unsigned int buffLeng)
{
    error_t err;

    if ((topic == NULL) || (buff == NULL))
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    err = mqttClientPublish(&gs_ctx.mqtt, topic, buff, (size_t)buffLeng,
                            APP_MQTT_CONN_MNG_QOS, FALSE, NULL);
    if (err != NO_ERROR)
    {
        appMqttConnMngNotify(APP_MQTT_STAT_PUB_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    appMqttConnMngNotify(APP_MQTT_STAT_PUB_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngIsConnected(void)
{
    return gs_ctx.isConnected;
}

int appMqttConnMngProcess(void)
{
    error_t err;

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_SUCCESS;
    }

    err = mqttClientTask(&gs_ctx.mqtt, (systime_t)0);
    if (err != NO_ERROR)
    {
        appMqttConnMngMarkLinkDown();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mqtt.state == MQTT_CLIENT_STATE_DISCONNECTED)
    {
        appMqttConnMngMarkLinkDown();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

/******************************** End Of File *********************************/
