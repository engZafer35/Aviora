/******************************************************************************
* #Author       : Zafer Satilmis
* #hype-man     : Nightwish - Nemo
* #Revision     : 1.0
* #Date         : Apr 6, 2026
* #File Name    : AppMqttMosquittoManager.c
*******************************************************************************/
/******************************************************************************
* MQTT transport (Eclipse Mosquitto client library). Implements AppMqttConnManager.h
* with the same semantics as AppMqttConnManager.c (CycloneTCP): return codes,
* callbacks, and process() behaviour. Link either this file or AppMqttConnManager.c,
* not both.
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include <mosquitto.h>
#include <string.h>

#include "AppMqttConnManager.h"

/****************************** MACRO DEFINITIONS *****************************/
#define APP_MQTT_CONN_MNG_RET_SUCCESS         (0)
#define APP_MQTT_CONN_MNG_RET_FAILURE         (-1)

#define APP_MQTT_CONN_MNG_KEEPALIVE_SEC       (60)
#define APP_MQTT_CONN_MNG_LOOP_TIMEOUT_MS     (250)
#define APP_MQTT_CONN_MNG_CONNECT_ATTEMPTS    (10) /* ~5 s waiting for CONNACK */

/******************************* TYPE DEFINITIONS *******************************/

typedef struct AppMqttConnMosqCtx_s
{
    struct mosquitto *mosq;
    volatile int isConnected;
    volatile int connectRcvd;
    /** Set only after appMqttConnMngConnectBroker returns success (avoids process() during handshake). */
    volatile int sessionReady;
    AppMqttConnMngConnInfoCb_t connCb;
    AppMqttConnMngListenCb_t publishCb;
} AppMqttConnMosqCtx_t;

/********************************** VARIABLES *********************************/
static AppMqttConnMosqCtx_t gs_ctx;

/***************************** STATIC FUNCTIONS  ******************************/
/**
 * @brief Tear down client after unexpected loss; notify CONNECTION_FAIL (Cyclone parity).
 *        Safe when broker already closed the socket (e.g. after on_disconnect).
 */
static void appMqttConnMngMarkLinkDown(void)
{
    if (gs_ctx.mosq == NULL)
    {
        return;
    }

    gs_ctx.isConnected = 0;
    gs_ctx.publishCb = NULL;

    (void)mosquitto_disconnect(gs_ctx.mosq);
    mosquitto_destroy(gs_ctx.mosq);
    gs_ctx.mosq = NULL;
    gs_ctx.sessionReady = 0;

    (void)mosquitto_lib_cleanup();

    gs_ctx.connCb(APP_MQTT_STAT_CONNECTION_FAIL);
}

static void appMqttConnMosqOnConnect(struct mosquitto *mosq, void *obj, int rc)
{
    (void)mosq;
    (void)obj;

    gs_ctx.connectRcvd = 1;

    if (0 == rc)
    {
        gs_ctx.isConnected = 1;
        gs_ctx.connCb(APP_MQTT_STAT_CONNECTION_OK);
    }
    else
    {
        gs_ctx.isConnected = 0;
        gs_ctx.connCb(APP_MQTT_STAT_CONNECTION_FAIL);
    }
}

static void appMqttConnMosqOnDisconnect(struct mosquitto *mosq, void *obj, int rc)
{
    gs_ctx.isConnected = 0;

    if(rc != 0)
    {
        gs_ctx.connCb(APP_MQTT_STAT_CONNECTION_FAIL);
        printf("Unexpected disconnection! rc=%d\n", rc);
        
    } else {
        printf("Disconnected cleanly\n");
    }   
}

static void appMqttConnMosqOnMessage(struct mosquitto *mosq, void *obj,
                                     const struct mosquitto_message *msg)
{
    const char *topic;
    const void *payload;
    unsigned int len;

    (void)mosq;
    (void)obj;

    if ((msg == NULL) || (gs_ctx.publishCb == NULL))
    {
        return;
    }

    topic = msg->topic;
    payload = msg->payload;
    len = (unsigned int)msg->payloadlen;

    if (topic == NULL)
    {
        return;
    }

    if ((len > 0U) && (payload == NULL))
    {
        return;
    }

    gs_ctx.publishCb(topic, payload, len);
}

static int appMqttConnMngApplyAuth(const char *clientName, const char *clientPsw)
{
    const char *user;
    const char *pass;
    int mr;

    user = ((clientName != NULL) && (clientName[0] != '\0')) ? clientName : "";
    pass = ((clientPsw != NULL) && (clientPsw[0] != '\0')) ? clientPsw : "";

    if ((user[0] == '\0') && (pass[0] == '\0'))
    {
        return APP_MQTT_CONN_MNG_RET_SUCCESS;
    }

    mr = mosquitto_username_pw_set(gs_ctx.mosq, user, pass);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

static void appMqttConnMosqAbortConnect(void)
{
    if (gs_ctx.mosq != NULL)
    {
        (void)mosquitto_disconnect(gs_ctx.mosq);
        mosquitto_destroy(gs_ctx.mosq);
        gs_ctx.mosq = NULL;
    }

    gs_ctx.isConnected = 0;
    gs_ctx.connectRcvd = 0;
    gs_ctx.sessionReady = 0;
    gs_ctx.connCb = NULL;
    gs_ctx.publishCb = NULL;

    (void)mosquitto_lib_cleanup();
}

/***************************** PUBLIC FUNCTIONS  ******************************/

int appMqttConnMngConnectBroker(const char *ip, unsigned short port,
                                const char *clientName, const char *clientPsw,
                                const char *clientId,
                                AppMqttConnMngConnInfoCb_t connCb,
                                AppMqttConnMngListenCb_t publishCb)
{
    int mr;
    int i;

    if ((ip == NULL) || (clientId == NULL) || (connCb == NULL) || (publishCb == NULL))
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 != gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mosq != NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_lib_init();
    if (mr != MOSQ_ERR_SUCCESS)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb = connCb;
    gs_ctx.publishCb = publishCb;
    gs_ctx.connectRcvd = 0;

    gs_ctx.mosq = mosquitto_new(clientId, true, NULL);
    if (gs_ctx.mosq == NULL)
    {
        gs_ctx.connCb = NULL;
        gs_ctx.publishCb = NULL;
        (void)mosquitto_lib_cleanup();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mosquitto_connect_callback_set(gs_ctx.mosq, appMqttConnMosqOnConnect);
    mosquitto_disconnect_callback_set(gs_ctx.mosq, appMqttConnMosqOnDisconnect);
    mosquitto_message_callback_set(gs_ctx.mosq, appMqttConnMosqOnMessage);

    if (APP_MQTT_CONN_MNG_RET_SUCCESS != appMqttConnMngApplyAuth(clientName, clientPsw))
    {
        appMqttConnMosqAbortConnect();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_connect(gs_ctx.mosq, ip, (int)port, APP_MQTT_CONN_MNG_KEEPALIVE_SEC);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        appMqttConnMosqAbortConnect();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    for (i = 0; i < APP_MQTT_CONN_MNG_CONNECT_ATTEMPTS; i++)
    {
        mr = mosquitto_loop(gs_ctx.mosq, APP_MQTT_CONN_MNG_LOOP_TIMEOUT_MS, 1);
        if (mr != MOSQ_ERR_SUCCESS)
        {
            appMqttConnMosqAbortConnect();
            return APP_MQTT_CONN_MNG_RET_FAILURE;
        }

        if (0 != gs_ctx.connectRcvd)
        {
            break;
        }
    }

    if (0 == gs_ctx.connectRcvd)
    {
        appMqttConnMosqAbortConnect();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        appMqttConnMosqAbortConnect();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.sessionReady = 1;

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngDisconnectBroker(void)
{
    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mosq == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb = NULL;
    gs_ctx.publishCb = NULL;

    (void)mosquitto_disconnect(gs_ctx.mosq);
    mosquitto_destroy(gs_ctx.mosq);
    gs_ctx.mosq = NULL;

    gs_ctx.isConnected = 0;
    gs_ctx.connectRcvd = 0;
    gs_ctx.sessionReady = 0;

    (void)mosquitto_lib_cleanup();

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngSubscribe(const char *topic)
{
    int mr;

    if (topic == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mosq == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_subscribe(gs_ctx.mosq, NULL, topic, 0);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        gs_ctx.connCb(APP_MQTT_STAT_SUBS_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb(APP_MQTT_STAT_SUBS_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngUnsubscribe(const char *topic)
{
    int mr;

    if (topic == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mosq == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_unsubscribe(gs_ctx.mosq, NULL, topic);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        gs_ctx.connCb(APP_MQTT_STAT_UNSUBS_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb(APP_MQTT_STAT_UNSUBS_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngPublish(const char *topic, const char *buff, unsigned int buffLeng)
{
    int mr;

    if ((topic == NULL) || (buff == NULL))
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (0 == gs_ctx.isConnected)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    if (gs_ctx.mosq == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_publish(gs_ctx.mosq, NULL, topic, (int)buffLeng, buff, 0, false);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        gs_ctx.connCb(APP_MQTT_STAT_PUB_FAIL);
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    gs_ctx.connCb(APP_MQTT_STAT_PUB_OK);
    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

int appMqttConnMngIsConnected(void)
{
    return gs_ctx.isConnected;
}

int appMqttConnMngProcess(void)
{
    int mr;

    if (gs_ctx.mosq == NULL)
    {
        return APP_MQTT_CONN_MNG_RET_SUCCESS;
    }

    /* Cyclone parity: idle / not yet fully up — do nothing destructive. */
    if (0 == gs_ctx.sessionReady)
    {
        return APP_MQTT_CONN_MNG_RET_SUCCESS;
    }

    /* Broker closed the session: on_disconnect cleared isConnected but client still exists. */
    if (0 == gs_ctx.isConnected)
    {
        appMqttConnMngMarkLinkDown();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    mr = mosquitto_loop(gs_ctx.mosq, APP_MQTT_CONN_MNG_LOOP_TIMEOUT_MS, 1);
    if (mr != MOSQ_ERR_SUCCESS)
    {
        appMqttConnMngMarkLinkDown();
        return APP_MQTT_CONN_MNG_RET_FAILURE;
    }

    return APP_MQTT_CONN_MNG_RET_SUCCESS;
}

/******************************** End Of File *********************************/
