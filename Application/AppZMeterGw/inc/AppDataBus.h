/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 29 Mar 2024 - 11:32:59
* #File Name    : AppDataBus.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __APP_DATA_BUS_H__
#define __APP_DATA_BUS_H__
/*********************************INCLUDES*************************************/
#include "Project_Conf.h"

#include "Midd_OSPort.h"
/******************************MACRO DEFINITIONS*******************************/

#define MAX_CLIENT_NUMBER   (10)

/*******************************TYPE DEFINITIONS ******************************/
/** @brief Packet Priority */
typedef enum
{
    EN_PRIORITY_LOW,
    EN_PRIORITY_MED,
    EN_PRIORITY_HIG,
    EN_PRIORITY_EMG
}DBUS_PRIORITY;

typedef enum
{
    EN_DBUS_TOPIC_NO        = 0X00, /* just in listener mode, */
    EN_DBUS_TOPIC_GSM       = 0X01,
    EN_DBUS_TOPIC_NETWORK   = 0X02,
    EN_DBUS_TOPIC_DISPLAY   = 0X04,
    EN_DBUS_TOPIC_DEVICE    = 0X08,
    EN_DBUS_TOPIC_TASK_MNG  = 0X10, /* Task MANAGER, it decides to reboot, So before reboot it informs everyone */

}DBUS_TOPICS;

typedef struct
{
    U32 dataLeng;
    U8  data[32];
}DBUS_PAYLOAD;

typedef struct
{
    U32             packetID;
    DBUS_TOPICS     topic;
    DBUS_PRIORITY   pri;        /* packet priority level */
    BOOL            retainFlag; /* If client ask the message manually, when this flag is 1, keep this packet as a last packet and send it to client. */
    DBUS_PAYLOAD    payload;

}DBUS_PACKET;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

RETURN_STATUS appDBusInit(void);

RETURN_STATUS appDBusRegister(DBUS_TOPICS listenTopic, S32 *clientID);

RETURN_STATUS appDBusUnregister(S32 clientID);

RETURN_STATUS appDBusPublish(S32 clientID, DBUS_PACKET *packet);

RETURN_STATUS appDBusReceive(S32 clientID, DBUS_PACKET *packet, U32 timeOut);

RETURN_STATUS appDBusRequest(S32 myClientID, DBUS_TOPICS topic);

#endif /* __APP_DATA_BUS_H__ */

/********************************* End Of File ********************************/
