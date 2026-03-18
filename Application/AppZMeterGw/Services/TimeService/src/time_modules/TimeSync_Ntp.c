/**
 * @file TimeSync_Ntp.c
 * @brief Minimal NTP client (UDP) using net_config.h socket macros
 */
#define SHOW_PAGE_DBG_MSG  (DISABLE)

#include "Project_Conf.h"

#include "ZDebug.h"

#include "net_config.h"

#include <string.h>
#include <stdio.h>

#define NTP_PORT_DEFAULT    (123)
#define NTP_PACKET_SIZE     (48)
#define NTP_EPOCH_OFFSET    (2208988800UL) /* seconds between 1900 and 1970 */

static char gs_ntpHost[128] = "pool.ntp.org";
static U16  gs_ntpPort = NTP_PORT_DEFAULT;

RETURN_STATUS appTimeNtpSetServer(const char *host, U16 port)
{
    if (IS_NULL_PTR(host) || host[0] == '\0')
    {
        return FAILURE;
    }

    strncpy(gs_ntpHost, host, sizeof(gs_ntpHost) - 1);
    gs_ntpHost[sizeof(gs_ntpHost) - 1] = '\0';
    gs_ntpPort = (port == 0) ? (U16)NTP_PORT_DEFAULT : port;
    return SUCCESS;
}

static RETURN_STATUS udpNtpQuery(U32 *outEpochUtc)
{
    if (IS_NULL_PTR(outEpochUtc))
    {
        return FAILURE;
    }

    struct addrinfo hints;
    struct addrinfo *res = NULL;
    char portStr[8];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    (void)snprintf(portStr, sizeof(portStr), "%u", (unsigned)gs_ntpPort);

    if (0 != getaddrinfo(gs_ntpHost, portStr, &hints, &res))
    {
        DEBUG_ERROR("->[E] TimeSrv: NTP DNS failed (%s)", gs_ntpHost);
        return FAILURE;
    }

    int sock = SOCKET(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        freeaddrinfo(res);
        DEBUG_ERROR("->[E] TimeSrv: NTP socket failed");
        return FAILURE;
    }

    /* Set receive timeout (best-effort) */
#if defined(SO_RCVTIMEO)
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    (void)SETSOCKOPT(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    U8 pkt[NTP_PACKET_SIZE];
    memset(pkt, 0, sizeof(pkt));
    /* LI=0, VN=4, Mode=3(client) */
    pkt[0] = 0x23;

    if ((int)sizeof(pkt) != SENDTO(sock, pkt, sizeof(pkt), 0, res->ai_addr, (SOCKLEN_t)res->ai_addrlen))
    {
        CLOSESOCKET(sock);
        freeaddrinfo(res);
        DEBUG_ERROR("->[E] TimeSrv: NTP sendto failed");
        return FAILURE;
    }

    struct sockaddr_storage from;
    SOCKLEN_t fromLen = (SOCKLEN_t)sizeof(from);
    int r = RECVFROM(sock, pkt, sizeof(pkt), 0, (struct sockaddr *)&from, &fromLen);

    CLOSESOCKET(sock);
    freeaddrinfo(res);

    if (r < (int)NTP_PACKET_SIZE)
    {
        DEBUG_ERROR("->[E] TimeSrv: NTP recv failed");
        return FAILURE;
    }

    /* Transmit Timestamp seconds field at bytes 40..43 (big-endian) */
    U32 sec1900 = ((U32)pkt[40] << 24) | ((U32)pkt[41] << 16) | ((U32)pkt[42] << 8) | (U32)pkt[43];
    if (sec1900 < NTP_EPOCH_OFFSET)
    {
        return FAILURE;
    }
    *outEpochUtc = sec1900 - NTP_EPOCH_OFFSET;
    return SUCCESS;
}

RETURN_STATUS appTimeNtpGetEpochUtc(U32 *outEpochUtc)
{
    return udpNtpQuery(outEpochUtc);
}

