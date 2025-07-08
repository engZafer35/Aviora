/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jul 8, 2025 - 10:17:04 AM
* #File Name    : MiddMqtt.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef	__MIDD_MQTT_H__
#define __MIDD_MQTT_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/

typedef enum ConnStatus_
{
    EN_MQTT_STAT_CONNECTION_OK,
    EN_MQTT_STAT_CONNECTION_FAIL,

    EN_MQTT_STAT_SUBS_OK,
    EN_MQTT_STAT_SUBS_FAIL,

    EN_MQTT_STAT_UNSUBS_OK,
    EN_MQTT_STAT_UNSUBS_FAIL,

    EN_MQTT_STAT_PUB_OK,
    EN_MQTT_STAT_PUB_FAIL,

}ConnStatus;

typedef void (*ListenCb)(const char *topic, const void *rcvBuff, unsigned int buffLeng);
typedef void (*ConnInfoCb)(ConnStatus stat);

#define MAX_TOPIC_NAME_LENG   (64)  //BYTE
#define MAX_TOPIC_NUM         (5)  //MAX TOPIC NUMBER
/************************* GLOBAL VARIBALE REFERENCES *************************/


/************************* GLOBAL FUNCTION DEFINITIONS ************************/

/**
 * @brief   connect mqtt broker
 * @param   nameID, broker name. it is nickname for the broker. max 15 character
 * @param   ip, broker ip, in string format like "127.0.0.1"
 * @param   port broker port number
 * @param   clientName, set NULL if it doesnt exist
 * @param   clientPsw,  password, set NULL if it doesnt exist
 * @param   connCb,  connection callback function. The user is informed over this callback function. Check ConnStatus enum list
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttConnectBroker(const char *brokerName, const char *ip, unsigned short int port, const char* clientName, const char* clientPsw, ConnInfoCb connCb);

/**
 * @brief   disconnectf from mqtt broker
 * @param   nameID, broker name.
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttDisconnectBroker(const char *brokerName);

/**
 * @brief   subscribe to topic
 * @param   nameID, broker name. use the same name with connect broker
 * @param   topic
 * @param   cb, set callback function. When a data received, this function is called
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttSubscribe(const char *brokerName, const char *topic, ListenCb cb);

/**
 * @brief   unsubscribe from topic
 * @param   nameID, broker name. use the same name with connect broker
 * @param   topic
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttUnsubscribe(const char *brokerName, const char *topic);

/**
 * @brief   publish data
 * @param   nameID, broker name. use the same name with connect broker
 * @param   topic
 * @param   buff, data buffer
 * @param   buffLeng, data buffer leng. it could be max 1024
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttPublish(const char *brokerName, const char *topic, const char *buff, int buffLeng);

/**
 * @brief   get connection status
 * @param   nameID, broker name. use the same name with connect broker
 * @return if everything is OK, return SUCCES
 *         otherwise return FAILURE
 */
RETURN_STATUS middMqttIsConnected(const char *brokerName);

#endif /* __MIDD_MQTT_H__ */

/********************************* End Of File ********************************/
