/******************************************************************************
* #Author       : Zafer Satilmis
* #hype-man     : In Spite - The Miserables
* #Revision     : 2.0
* #Date         : Mar 31, 2026
* #File Name    : AppMqttConnManager.h
*******************************************************************************/

/******************************************************************************
* CycloneTCP mqtt_client wrapper: one connection, no broker nickname or topic
* table — that state lives in AppMqttConnService (or another owner). Call
* appMqttConnMngProcess() while connected for RX and keep-alive.
******************************************************************************/
#ifndef __APP_MQTT_CONN_MANAGER_H__
#define __APP_MQTT_CONN_MANAGER_H__

/****************************** MACRO DEFINITIONS *******************************/

#define APP_MQTT_CONN_MNG_MAX_TOPIC_NAME_LEN   (64U)
#define APP_MQTT_CONN_MNG_MAX_TOPIC_NUM        (2U)
#define APP_MQTT_CONN_MNG_CLIENT_ID_BUF_LEN    (24U) /* MQTT_CLIENT_MAX_ID_LEN + 1 */

/******************************* TYPE DEFINITIONS *******************************/

typedef enum AppMqttConnStatus_e
{
    APP_MQTT_STAT_CONNECTION_OK = 0,
    APP_MQTT_STAT_CONNECTION_FAIL,

    APP_MQTT_STAT_SUBS_OK,
    APP_MQTT_STAT_SUBS_FAIL,

    APP_MQTT_STAT_UNSUBS_OK,
    APP_MQTT_STAT_UNSUBS_FAIL,

    APP_MQTT_STAT_PUB_OK,
    APP_MQTT_STAT_PUB_FAIL,
} AppMqttConnStatus_t;

typedef void (*AppMqttConnMngListenCb_t)(const char *topic, const void *rcvBuff,
                                         unsigned int buffLeng);
typedef void (*AppMqttConnMngConnInfoCb_t)(AppMqttConnStatus_t stat);

/************************* GLOBAL FUNCTION DEFINITIONS **************************/

/**
 * @brief Connect (blocks until CONNACK or failure).
 * @param clientId MQTT Client Identifier (owner builds, e.g. from broker nickname).
 * @param publishCb Called for every incoming PUBLISH after subscribe.
 */
int appMqttConnMngConnectBroker(const char *ip, unsigned short port,
                                const char *clientName, const char *clientPsw,
                                const char *clientId,
                                AppMqttConnMngConnInfoCb_t connCb,
                                AppMqttConnMngListenCb_t publishCb);

/**
 * @brief Disconnect and deinit.
 */
int appMqttConnMngDisconnectBroker(void);

/**
 * @brief Subscribe to a topic.  Topic string is copied by caller, can be stack-allocated.
 */
int appMqttConnMngSubscribe(const char *topic);

/**
 * @brief Unsubscribe from a topic.
 */
int appMqttConnMngUnsubscribe(const char *topic);

/**
 * @brief Publish a message to a topic.  Topic and payload strings are copied by caller, can be stack-allocated.
 */
int appMqttConnMngPublish(const char *topic, const char *buff, unsigned int buffLeng);

/**
 * @brief Check if connected to the broker.
 * @return 1 if connected, 0 otherwise.
 */
int appMqttConnMngIsConnected(void);

/**
 * @brief Process MQTT client tasks: receive messages, handle keep-alive, etc.
 *        Call this periodically while connected.
 * @return APP_MQTT_CONN_MNG_RET_SUCCESS if still connected, APP_MQTT_CONN_MNG_RET_FAILURE if connection lost.
 */
int appMqttConnMngProcess(void);

#endif /* __APP_MQTT_CONN_MANAGER_H__ */

/********************************* End Of File ********************************/
