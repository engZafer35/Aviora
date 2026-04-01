/******************************************************************************
* #Author       : Zafer Satilmis
* #hype-man     : In Spite - Killing A Friend
* #Revision     : 1.0
* #Date         : Mar 31, 2026
* #File Name    : AppMqttConnService.h
*******************************************************************************/

/******************************************************************************
* MQTT connection service (CycloneTCP via AppMqttConnManager): dedicated task,
* keep-alive/process loop, auto-reconnect on loss. Same role as
* massProtocol/mass_conn_thread_entry (lwIP + luna_mqtt_*).
*
* Incoming callback matches AppTcpConnManager IncomingMsngCb_t shape; first
* argument is the MQTT topic name instead of a channel name.
******************************************************************************/
#ifndef __APP_MQTT_CONN_SERVICE_H__
#define __APP_MQTT_CONN_SERVICE_H__

/****************************** MACRO DEFINITIONS *******************************/

/** Service-side broker nickname; used to build MQTT clientId (see AppMqttConnService.c). */
#define APP_MQTT_CONN_SERVICE_BROKER_NAME   "ZMQTT"

/******************************* TYPE DEFINITIONS *******************************/

/**
 * @brief Delivered for each received PUBLISH on a subscribed topic.
 * @param topic   MQTT topic name (same role as channel in TCP manager).
 * @param data    Payload (not necessarily null-terminated).
 * @param dataLength Payload length in bytes.
 */
typedef void (*AppMqttConnServiceIncomingCb_t)(const char *topic, const char *data,
                                               unsigned int dataLength);

/**
 * @brief Optional: link/session notification.
 * @param isUp  non-zero when MQTT is connected and initial subscriptions succeeded;
 *              zero when disconnected or after a loss before reconnect completes.
 */
typedef void (*AppMqttConnServiceLinkCb_t)(int isUp);

/************************* GLOBAL FUNCTION DEFINITIONS **************************/

/**
 * @brief Start the MQTT service task (does not connect until appMqttConnServiceRequestConnect).
 * @param brokerIP    Broker IPv4 string.
 * @param brokerPort  TCP port (e.g. 1883).
 * @param userName    MQTT user; NULL or empty = no username in CONNECT.
 * @param psw         MQTT password; NULL or empty = no password in CONNECT.
 * @param subsTopic   First subscription topic (copied internally).
 * @param incomingCb  Required: incoming publishes (topic = MQTT topic).
 * @param linkCb      Optional: link up/down; NULL if unused.
 * @return 0 on success, -1 on error (e.g. task not created).
 */
int appMqttConnServiceStart(const char *brokerIP, int brokerPort,
                            const char *userName, const char *psw,
                            const char *subsTopic,
                            AppMqttConnServiceIncomingCb_t incomingCb,
                            AppMqttConnServiceLinkCb_t linkCb);

/**
 * @brief Request service shutdown (non-blocking). The worker task disconnects,
 *        releases resources, and deletes itself. Wait until the service is fully stopped
 *        before calling appMqttConnServiceStart again (e.g. poll or delay if needed).
 */
int appMqttConnServiceStop(void);

/**
 * @brief Non-zero if currently connected to the broker.
 */
int appMqttConnServiceIsConnected(void);

/**
 * @brief Request connection (handled asynchronously in the service task).
 */
int appMqttConnServiceRequestConnect(void);

/**
 * @brief Request disconnection when idle (mirrors mass_conn_request_disconnect).
 * @return
 */
int appMqttConnServiceRequestDisconnect(void);

/**
 * @brief Publish QoS 0 (requires connection).
 * @param topic         MQTT topic string (e.g. "avi/request" or "myhome/sensor1/data").
 * @param data          Payload bytes (not necessarily null-terminated).
 * @param dataLength    Payload length in bytes.
 * @return 0 on success, -1 on failure (e.g. not connected).
 */
int appMqttConnServicePublish(const char *topic, const char *data, unsigned int dataLength);

/**
 * @brief Add a subscription on a free slot (same pattern as mass_conn_subs).
 *        If already connected, subscribes immediately.
 * @param topic MQTT topic string (e.g. "avi/request" or "myhome/sensor1/data").
 * @return 0 on success, -1 on failure (e.g. no free slot 
 */
int appMqttConnServiceSubscribe(const char *topic);

#endif /* __APP_MQTT_CONN_SERVICE_H__ */

/********************************* End Of File ********************************/
