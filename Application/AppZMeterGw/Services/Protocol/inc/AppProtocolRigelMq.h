/******************************************************************************
* #Author       : Zafer Satilmis
* #Hhype-man    : KRONIK - CANIN CEHENNEME V2
* #Revision     : 1.0
* #Date         : Apr 1, 2026
* #File Name    : AppProtocolRigelMq.h
*******************************************************************************/

/******************************************************************************
* JSON-over-MQTT protocol handler with session management for IoT meter gateway.
* Uses MQTT for communication: default subscribe/publish topics are \c RG_SESSION_REQUEST_TOPIC /
* \c RG_SESSION_RESPONSE_TOPIC; \c setting / \c request.mqtt can override and persist them.
* Session-based message processing similar to OrionTLV but with JSON payloads.
******************************************************************************/
#ifndef __APP_PROTOCOL_RIGEL_MQ_H__
#define __APP_PROTOCOL_RIGEL_MQ_H__

/*********************************INCLUDES*************************************/
#include "Project_Conf.h"
#include "AppMqttConnService.h"
#include <stdint.h>
#include <stdbool.h>

/******************************MACRO DEFINITIONS*******************************/

#define RG_SESSION_FLAG               "AVI"
#define RG_SESSION_BRAND              "AVI"
#define RG_SESSION_MODEL              "AVIO2622"

#define RG_SESSION_PACKET_MAX_SIZE    (1024)
#define RG_SESSION_DATA_CHUNK_SIZE    (700)
#define RG_SESSION_MAX_SESSIONS       (8)
#define RG_SESSION_TIMEOUT_MS         (10000)  /* 10 seconds */

/** Alive: main task loop runs every 100 ms; value is tick count (9000 × 100 ms = 15 min). */
#define RG_SESSION_ALIVE_INTERVAL_S   (9000)

/** Max length for runtime MQTT subscribe/publish topic strings (see \c setting / Server / mqtt). */
#define RG_SESSION_TOPIC_MAX          (64)

#define RG_SESSION_REGISTER_FILE      "rg_session_register.dat"
#define RG_SESSION_SERVER_FILE        "rg_session_server.dat"

/* Function codes */
#define RG_FUNC_IDENT         1
#define RG_FUNC_ALIVE         2
#define RG_FUNC_LOG           3
#define RG_FUNC_READOUT       4
#define RG_FUNC_LOADPROFILE   5
#define RG_FUNC_DIRECTIVE_LIST 6
#define RG_FUNC_ACK           8
#define RG_FUNC_NACK          9
#define RG_FUNC_SETTING       10
#define RG_FUNC_FWUPDATE      11
#define RG_FUNC_DIRECTIVE_ADD 12
#define RG_FUNC_DIRECTIVE_DELETE 13

/* ── Event types ── */
typedef enum
{
    RG_SESSION_EVENT_ALIVE,
    RG_SESSION_EVENT_FW_UPGRADE_SUCCESS,
} RG_SessionEventType_t;

/*******************************TYPE DEFINITIONS ******************************/

typedef struct
{
    char ch[16];
    char payload[RG_SESSION_PACKET_MAX_SIZE];
} RG_SessionMsgQueue_t;

typedef enum
{
    RG_SESSION_STATE_DONE = 0,
    RG_SESSION_STATE_ERROR,
    RG_SESSION_STATE_TIMEOUT,
} RG_SessionState_t;

typedef struct RG_Session_s RG_Session_t;

typedef void (*RG_SessionCompleteCb_t)(RG_Session_t *session, RG_SessionState_t state, void *arg);

struct RG_Session_s
{
    bool        inUse;
    uint8_t     function;
    uint16_t    transNumber;
    uint32_t    step;
    uint32_t    timeCnt;
    char        rxBuf[RG_SESSION_PACKET_MAX_SIZE];
    uint16_t    rxLen;
    bool        rxReady;
    int         pendingJobIdx;
    RG_SessionCompleteCb_t onCompleteFunc;
};

/************************* GLOBAL FUNCTION DEFINITIONS **************************/

/**
 * @brief  Initialise the RigelMq protocol module.
 * @param  serialNumber  Device serial number (null-terminated).
 *         MQTT broker, topics, device/pull endpoints: loaded from \c RG_SESSION_SERVER_FILE if present;
 *         otherwise defaults from \c RG_RIGEL_DEFAULT_* macros in this header and the file is created.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolRigelMqInit(const char *serialNumber);

/**
 * @brief  Start the ZD session protocol task (connects to MQTT and enters state machine).
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolRigelMqStart(void);

/**
 * @brief  Stop the ZD session protocol task and disconnect MQTT.
 * @return SUCCESS / FAILURE
 */
RETURN_STATUS appProtocolRigelMqStop(void);

/**
 * @brief   Post an asynchronous event to the ZD session protocol service.
 * @param   type   event type
 * @return  TRUE if queued, FALSE if queue full
 */
BOOL appProtocolRigelMqSendEvent(RG_SessionEventType_t type);

#endif /* __APP_PROTOCOL_RIGEL_MQ_H__ */
