/******************************************************************************
* #Author       :
* #Revision     : 1.0
* #Date         : Jan 13, 2025
* #File Name    : mqtt_connection_manager.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include "MiddMqtt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/****************************** MACRO DEFINITIONS *****************************/
#define USE_MOSQUITTO

#ifdef USE_MOSQUITTO
  #include <mosquitto.h>
#elif defined(USE_CYCLONETCP)
#include "net_config.h"
#include "mqtt_client.h"
#include "net.h"
#else
  #error "Define USE_MOSQUITTO or USE_CYCLONETCP"
#endif

#define MAX_BROKER     5
#define MAX_NAME_LEN   16

/******************************* TYPE DEFINITIONS *****************************/
typedef struct {
    char         name[MAX_NAME_LEN];
    bool         used;
    bool         connected;
    ConnInfoCb   connCb;
    ListenCb     listenCb;

#ifdef USE_MOSQUITTO
    struct mosquitto *mosq;
#elif defined(USE_CYCLONETCP)
    MqttClient    client;
    SOCKET      sock;
#endif
} BrokerCtx;

/********************************** VARIABLES *********************************/
static BrokerCtx brokers[MAX_BROKER] = {0};

/***************************** STATIC FUNCTIONS  ******************************/
static BrokerCtx *getBroker(const char *name)
{
    for(int i=0; i < MAX_BROKER; i++)
    {
        if((TRUE == brokers[i].used) && (0 == strcmp(brokers[i].name,name)))
        {
            return &brokers[i];
        }
    }

    return NULL;
}
static BrokerCtx *allocBroker(const char *name)
{
    for(int i=0; i < MAX_BROKER; i++)
    {
        if(!brokers[i].used)
        {
            strncpy(brokers[i].name,name,MAX_NAME_LEN-1);
            brokers[i].used = TRUE;

            return &brokers[i];
        }
    }
    return NULL;
}

#ifdef USE_MOSQUITTO

static void mosqOnConnect(struct mosquitto *mosq, void *obj, int rc)
{
    BrokerCtx *broker = (BrokerCtx*)obj;

    broker->connected = (rc == 0);
    if(broker->connCb)
    {
        broker->connCb(rc == 0 ? EN_MQTT_STAT_CONNECTION_OK : EN_MQTT_STAT_CONNECTION_FAIL);
    }
}
static void mosqOnMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    BrokerCtx *broker = (BrokerCtx*)obj;

    if(broker->listenCb)
    {
        broker->listenCb(msg->topic, msg->payload, (unsigned int)msg->payloadlen);
    }
}

#endif

#ifdef USE_CYCLONETCP

static void cycloneOnConn(MqttClient *client, void *context, error_t err) {
    BrokerCtx *broker = (BrokerCtx*)context;
    broker->connected = (err == NO_ERROR);
    if(broker->connCb){
        broker->connCb(err == NO_ERROR ? EN_MQTT_STAT_CONNECTION_OK : EN_MQTT_STAT_CONNECTION_FAIL);
    }
}
static void cycloneOnMsg(MqttClient *client, const char *topic, void *payload, size_t len, void *context) {
    BrokerCtx *broker = (BrokerCtx*)context;
    if(broker->listenCb){
        broker->listenCb(topic, payload, (unsigned int)len);
    }
}

#endif


/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS middMqttConnectBroker(const char *brokerName, const char *ip,
                                    unsigned short port,
                                    const char *clientName,
                                    const char *clientPsw,
                                    ConnInfoCb connCb)
{
    if(NULL == getBroker(brokerName))
	{
		BrokerCtx *broker = allocBroker(brokerName);

		if(NULL == broker)
		{
			return FAILURE;
		}

		broker->connCb = connCb;

#ifdef USE_MOSQUITTO
		mosquitto_lib_init();
		broker->mosq = mosquitto_new(clientName, TRUE, broker);

		if(!broker->mosq)
		{
		    return FAILURE;
		}

		mosquitto_connect_callback_set(broker->mosq, mosqOnConnect);
		mosquitto_message_callback_set(broker->mosq, mosqOnMessage);

		if(clientName && clientPsw)
		{
			mosquitto_username_pw_set(broker->mosq, clientName, clientPsw);
		}

		if(mosquitto_connect(broker->mosq, ip, port, 60) != MOSQ_ERR_SUCCESS)
		{
			return FAILURE;
		}
		mosquitto_loop_start(broker->mosq);

#elif defined(USE_CYCLONETCP)
		netSocketOpen(&broker->sock, SOCKET_TYPE_STREAM, SOCKET_IPPROTO_TCP);
		if(netSocketConnect(&broker->sock, ip, port, 5000) != NO_ERROR)
        {
		    return FAILURE;
        }

		mqttClientInit(&broker->client, &broker->sock);
		mqttClientSetConnectionCallback(&broker->client, cycloneOnConn, b);
		mqttClientSetDefaultMessageHandler(&broker->client, cycloneOnMsg, b);

		if(clientName && clientPsw)
		{
			mqttClientSetAuthInfo(&broker->client, clientName, clientPsw);
		}
		if(mqttClientConnect(&broker->client, "client", TRUE) != NO_ERROR)
		{
			return FAILURE;
		}
		broker->connected = TRUE;
#endif

	}

    return SUCCESS;
}

RETURN_STATUS middMqttDisconnectBroker(const char *brokerName)
{
    BrokerCtx *broker = getBroker(brokerName);

    if(NULL == broker)
    {
        return FAILURE;
    }

#ifdef USE_MOSQUITTO
    mosquitto_disconnect(broker->mosq);
    mosquitto_loop_stop(broker->mosq, TRUE);
    mosquitto_destroy(broker->mosq);
    mosquitto_lib_cleanup();

#elif defined(USE_CYCLONETCP)
    mqttClientDisconnect(&broker->client);
    netSocketClose(&broker->sock);
#endif

    memset(broker, 0, sizeof(*broker));
    return SUCCESS;
}

RETURN_STATUS middMqttSubscribe(const char *brokerName, const char *topic, ListenCb cb)
{
    BrokerCtx *broker = getBroker(brokerName);

    if(!broker || !broker->connected)
    {
        return FAILURE;
    }

    broker->listenCb = cb;
#ifdef USE_MOSQUITTO
    if(mosquitto_subscribe(broker->mosq, NULL, topic, 0)!=MOSQ_ERR_SUCCESS)
    {
        broker->connCb(EN_MQTT_STAT_SUBS_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_SUBS_OK);

#elif defined(USE_CYCLONETCP)
    if(mqttClientSubscribe(&broker->client, topic, MQTT_QOS_0)!=NO_ERROR)
    {
        broker->connCb(EN_MQTT_STAT_SUBS_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_SUBS_OK);
#endif

    return SUCCESS;
}

RETURN_STATUS middMqttUnsubscribe(const char *brokerName, const char *topic)
{
    BrokerCtx *broker = getBroker(brokerName);

    if(!broker || !broker->connected)
    {
        return FAILURE;
    }

#ifdef USE_MOSQUITTO
    if(mosquitto_unsubscribe(broker->mosq, NULL, topic)!=MOSQ_ERR_SUCCESS)
    {
        broker->connCb(EN_MQTT_STAT_UNSUBS_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_UNSUBS_OK);

#elif defined(USE_CYCLONETCP)
    if(mqttClientUnsubscribe(&broker->client, topic)!=NO_ERROR)
    {
        broker->connCb(EN_MQTT_STAT_UNSUBS_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_UNSUBS_OK);
#endif

    return SUCCESS;
}

RETURN_STATUS middMqttPublish(const char *brokerName, const char *topic, const char *buff, int buffLeng)
{
    BrokerCtx *broker = getBroker(brokerName);
    if(!broker || !broker->connected) return FAILURE;

#ifdef USE_MOSQUITTO
    if(mosquitto_publish(broker->mosq, NULL, topic, buffLeng, buff, 0, false)!=MOSQ_ERR_SUCCESS)
    {
        broker->connCb(EN_MQTT_STAT_PUB_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_PUB_OK);

#elif defined(USE_CYCLONETCP)
    if(mqttClientPublish(&broker->client, topic, buff, buffLeng, MQTT_QOS_0, false)!=NO_ERROR)
    {
        broker->connCb(EN_MQTT_STAT_PUB_FAIL);
        return FAILURE;
    }
    broker->connCb(EN_MQTT_STAT_PUB_OK);
#endif

    return SUCCESS;
}

RETURN_STATUS middMqttIsConnected(const char *brokerName)
{
    BrokerCtx *broker = getBroker(brokerName);

    return (broker && broker->connected) ? SUCCESS : FAILURE;
}
