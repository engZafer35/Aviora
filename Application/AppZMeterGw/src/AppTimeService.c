/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : 26 Mar 2024 - 17:21:38
* #File Name    : AppTimeService.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "AppTimeService.h"

#include "core/bsd_socket.h"
#include "core/net.h"

#include "AppGlobalVariables.h"
#include "AppLogRecorder.h"

#include "MiddEventTimer.h"
/****************************** MACRO DEFINITIONS *****************************/
// Difference between Jan 1, 1900 and Jan 1, 1970
#define UNIX_OFFSET 2208988800L

#define NTP_DEFAULT_PORT "123"

// Flags 00|100|011 for li=0, vn=4, mode=3
#define NTP_FLAGS 0x23
/******************************* TYPE DEFINITIONS *****************************/
  typedef struct
  {

    U8 li_vn_mode;      // Eight bits. li, vn, and mode.
                             // li.   Two bits.   Leap indicator.
                             // vn.   Three bits. Version number of the protocol.
                             // mode. Three bits. Client will pick mode 3 for client.

    U8 stratum;         // Eight bits. Stratum level of the local clock.
    U8 poll;            // Eight bits. Maximum interval between successive messages.
    U8 precision;       // Eight bits. Precision of the local clock.

    U32 rootDelay;      // 32 bits. Total round trip delay time.
    U32 rootDispersion; // 32 bits. Max error aloud from primary clock source.
    U32 refId;          // 32 bits. Reference clock identifier.

    U32 refTm_s;        // 32 bits. Reference time-stamp seconds.
    U32 refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

    U32 origTm_s;       // 32 bits. Originate time-stamp seconds.
    U32 origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

    U32 rxTm_s;         // 32 bits. Received time-stamp seconds.
    U32 rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

    U32 txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
    U32 txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

  } ntp_packet;              // Total: 384 bits or 48 bytes.
/********************************** VARIABLES *********************************/
static int gs_sockfd;
/***************************** STATIC FUNCTIONS  ******************************/
#if (APP_TIME_SERVICE_NTP == ENABLE)

#define NTP_TIMESTAMP_DELTA 2208988800ull

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) // (li   & 11 000 000) >> 6
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) // (vn   & 00 111 000) >> 3
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) // (mode & 00 000 111) >> 0

static RETURN_STATUS initNTP(const char *ntpServer, U32 ntpPort)
{
    SOCKADDR_IN serv_addr; // Server address data structure.
    struct hostent *server;      // Server data structure.

    gs_sockfd = SOCKET(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (gs_sockfd < 0)
    {
        DEBUG_ERROR("->[E] ERROR opening socket" );
        return  FAILURE;
    }

    server = GETHOSTBYNAME((const char*)ntpServer); // Convert URL to IP.
    if (server == NULL )
    {
        DEBUG_ERROR("->[E] ERROR, no such host");
        return  FAILURE;
    }

    bzero( ( char* ) &serv_addr, sizeof( serv_addr ) );

    serv_addr.sin_family = AF_INET;

    // Copy the server's IP address to the server address structure.
    bcopy( ( char* )server->h_addr, ( char* ) &serv_addr.sin_addr.s_addr, server->h_length );

    // Convert the port number integer to network big-endian style and save it to the server address structure.
    serv_addr.sin_port = htons(ntpPort);

    // Call up the server using its IP address and port number.
    if (CONNECT( gs_sockfd, ( SOCKADDR * ) &serv_addr, sizeof( serv_addr) ) < 0 )
    {
        DEBUG_ERROR("->[E] ERROR connecting" );
        return  FAILURE;
    }

    return SUCCESS;
}

static RETURN_STATUS getTimeFromNTP(struct tm *timeStr)
{
    ntp_packet packet;

    memset(&packet, 0, sizeof(ntp_packet));

    packet.li_vn_mode = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.

    if (0 > SEND( gs_sockfd, (char*)&packet, sizeof( ntp_packet ), 0))
    {
        DEBUG_ERROR("->[E] ERROR writing to socket" );
        return FAILURE;
    }

    if (0 >= RECV(gs_sockfd, (char*)&packet, sizeof( ntp_packet ), 0))
    {
        DEBUG_ERROR("->[E] ERROR reading from socket");
        return FAILURE;
    }

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
    packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );
    *timeStr = *localtime(&txTm);

    printf(" %d: %d :%d - %d \n", timeStr->tm_mday, timeStr->tm_mon+1, timeStr->tm_year+1900, timeStr->tm_wday);
    return SUCCESS;
}

static RETURN_STATUS initRTC(void)
{
    return SUCCESS;
}

static RETURN_STATUS setRTCTime(const struct tm *time)
{
    return SUCCESS;
}

static void getRTCTime(struct tm *timeStr)
{
    /*todo:
     * get time from internal rtc register. If the chip doesn't have internal
     * rtc chip set 1 sec. timer to update time value. Just read rtc chip in 5 min period
     * to sync/check time.
     */

    time_t     now;

    // Get current time
    time(&now);

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    *timeStr = *localtime(&now);
}

#endif

static void tsTimerCb (void)
{
    struct tm tmpTime;
    printf(" -------<<Tcb>>> \n");

    if (SUCCESS == getTimeFromNTP(&tmpTime))
    {
        setRTCTime(&tmpTime);
    }
}
/***************************** PUBLIC FUNCTIONS  ******************************/
RETURN_STATUS appTimeServiceInit(const char *ntpServer, U32 ntpPort)
{
    RETURN_STATUS retVal;

#if (APP_TIME_SERVICE_NTP == ENABLE)

    if (FAILURE == initNTP(ntpServer, ntpPort))
    {
        DEBUG_ERROR("->[E] NTP init ERROR");
        appLogRec(g_sysLoggerID, "Error: NTP could not be initialized");
        retVal = FAILURE;
    }
    else
    {
        S32 timerID;
        middEventTimerRegister(&timerID, tsTimerCb, WAIT_2_SEC , TRUE);
        middEventTimerStart(timerID);
    }

#endif

    if (FAILURE == initRTC())
    {
        DEBUG_ERROR("->[E] RTC init ERROR");
        appLogRec(g_sysLoggerID, "Error: RTC could not be initialized");
        retVal |= FAILURE; //keep ntp initialize return value.
    }

    return SUCCESS;
}

void appTimeServiceGetTime(struct tm *tm)
{
    if (IS_SAFELY_PTR(tm))
    {
        getRTCTime(tm);
    }
}

#if (APP_TIME_SERVICE_NTP == DISABLE)

RETURN_STATUS appTimeServiceSetTime(const TS_Time *tm)
{
    return SUCCESS;
}

RETURN_STATUS appTimeServiceSetTimerDate(S32 *timerID, const TS_Time *tm, VoidCallback cb)
{
    return SUCCESS;
}

RETURN_STATUS appTimeServiceStopTimerDate(S32 timerID)
{
    return SUCCESS;
}
#endif
/******************************** End Of File *********************************/
