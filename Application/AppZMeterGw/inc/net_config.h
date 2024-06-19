/**
 * @file net_config.h
 * @brief CycloneTCP configuration file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.1.0
 **/

#ifndef _NET_CONFIG_H
#define _NET_CONFIG_H

//Trace level for TCP/IP stack debugging
#define MEM_TRACE_LEVEL          4
#define NIC_TRACE_LEVEL          4
#define ETH_TRACE_LEVEL          2
#define ARP_TRACE_LEVEL          2
#define IP_TRACE_LEVEL           2
#define IPV4_TRACE_LEVEL         2
#define IPV6_TRACE_LEVEL         2
#define ICMP_TRACE_LEVEL         2
#define IGMP_TRACE_LEVEL         2
#define ICMPV6_TRACE_LEVEL       2
#define MLD_TRACE_LEVEL          2
#define NDP_TRACE_LEVEL          2
#define UDP_TRACE_LEVEL          2
#define TCP_TRACE_LEVEL          2
#define SOCKET_TRACE_LEVEL       2
#define RAW_SOCKET_TRACE_LEVEL   2
#define BSD_SOCKET_TRACE_LEVEL   2
#define WEB_SOCKET_TRACE_LEVEL   2
#define AUTO_IP_TRACE_LEVEL      4
#define SLAAC_TRACE_LEVEL        4
#define DHCP_TRACE_LEVEL         4
#define DHCPV6_TRACE_LEVEL       4
#define DNS_TRACE_LEVEL          4
#define MDNS_TRACE_LEVEL         2
#define NBNS_TRACE_LEVEL         2
#define LLMNR_TRACE_LEVEL        2
#define COAP_TRACE_LEVEL         4
#define FTP_TRACE_LEVEL          5
#define HTTP_TRACE_LEVEL         5
#define MQTT_TRACE_LEVEL         4
#define MQTT_SN_TRACE_LEVEL      4
#define SMTP_TRACE_LEVEL         5
#define SNMP_TRACE_LEVEL         4
#define SNTP_TRACE_LEVEL         4
#define TFTP_TRACE_LEVEL         4
#define MODBUS_TRACE_LEVEL       4
#define PPP_TRACE_LEVEL          2

//Number of network adapters
#define NET_INTERFACE_COUNT 1

//Size of the MAC address filter
#define MAC_ADDR_FILTER_SIZE 12

//IPv4 support
#define IPV4_SUPPORT ENABLED
//Size of the IPv4 multicast filter
#define IPV4_MULTICAST_FILTER_SIZE 4

//IPv4 fragmentation support
#define IPV4_FRAG_SUPPORT ENABLED
//Maximum number of fragmented packets the host will accept
//and hold in the reassembly queue simultaneously
#define IPV4_MAX_FRAG_DATAGRAMS 4
//Maximum datagram size the host will accept when reassembling fragments
#define IPV4_MAX_FRAG_DATAGRAM_SIZE 8192

//Size of ARP cache
#define ARP_CACHE_SIZE 8
//Maximum number of packets waiting for address resolution to complete
#define ARP_MAX_PENDING_PACKETS 2

//IGMP host support
#define IGMP_HOST_SUPPORT DISABLE//ENABLED

//IPv6 support
#define IPV6_SUPPORT DISABLE
//Size of the IPv6 multicast filter
#define IPV6_MULTICAST_FILTER_SIZE 8

//IPv6 fragmentation support
#define IPV6_FRAG_SUPPORT ENABLED
//Maximum number of fragmented packets the host will accept
//and hold in the reassembly queue simultaneously
#define IPV6_MAX_FRAG_DATAGRAMS 4
//Maximum datagram size the host will accept when reassembling fragments
#define IPV6_MAX_FRAG_DATAGRAM_SIZE 8192

//MLD support
#define MLD_SUPPORT ENABLED

//Neighbor cache size
#define NDP_NEIGHBOR_CACHE_SIZE 8
//Destination cache size
#define NDP_DEST_CACHE_SIZE 8
//Maximum number of packets waiting for address resolution to complete
#define NDP_MAX_PENDING_PACKETS 2

//TCP support
#define TCP_SUPPORT ENABLED
//Default buffer size for transmission
#define TCP_DEFAULT_TX_BUFFER_SIZE (5430*2)
//Default buffer size for reception
#define TCP_DEFAULT_RX_BUFFER_SIZE (5430*2)
//Default SYN queue size for listening sockets
#define TCP_DEFAULT_SYN_QUEUE_SIZE 16//4 zafer
//Maximum number of retransmissions
#define TCP_MAX_RETRIES 5
//Selective acknowledgment support
#define TCP_SACK_SUPPORT ENABLED //DISABLED  zafer

//UDP support
#define UDP_SUPPORT ENABLED
//Receive queue depth for connectionless sockets
#define UDP_RX_QUEUE_SIZE 4

//Raw socket support
#define RAW_SOCKET_SUPPORT ENABLED//DISABLED
//Receive queue depth for raw sockets
#define RAW_SOCKET_RX_QUEUE_SIZE 16//4 zafer

//Number of sockets that can be opened simultaneously
#define SOCKET_MAX_COUNT 8 //2  Zafer

//LLMNR responder support
#define LLMNR_RESPONDER_SUPPORT ENABLED

//HTTP client support
#define HTTP_CLIENT_SUPPORT     ENABLED
#define NET_RTOS_SUPPORT        ENABLED  //support multi threading structure
#define BSD_SOCKET_SUPPORT      ENABLED
#define NBNS_CLIENT_SUPPORT     DISABLED
#define NBNS_RESPONDER_SUPPORT  DISABLED
#define PPP_SUPPORT             ENABLED
#define PING_SUPPORT            ENABLED

#if BSD_SOCKET_SUPPORT==ENABLED
/*
 * Use the following macro in linux OS, you can separate Cyclone bsd-func and linux bsd-func
 * Wiht this way, we can use both linux-bsd and Cyclone-bsd together
 */
//BSD socket related functions
#define SOCKET          c_socket
#define BIND            c_bind
#define CONNECT         c_connect
#define LISTEN          c_listen
#define ACCEPT          c_accept
#define SEND            c_send
#define SENDTO          c_sendto
#define SENDMSG         c_sendmsg
#define RECV            c_recv
#define RECVFROM        c_recvfrom
#define RECVMSG         c_recvmsg
#define getsockname     c_getsockname
#define getpeername     c_getpeername
#define SETSOCKOPT      c_setsockopt
#define GETSOCKOPT      c_getsockopt
#define IOCTLSOCKET     c_ioctlsocket
#define FCNTL           c_fcntl
#define SHUTDOWN        c_shutdown
#define CLOSESOCKET     c_closesocket
#define SELECT          c_select
#define GETHOSTNAME     c_gethostname
#define GETHOSTBYNAME   c_gethostbyname
#define gethostbyname_r c_gethostbyname_r
#define getaddrinfo     c_getaddrinfo
#define freeaddrinfo    c_freeaddrinfo
#define getnameinfo     c_getnameinfo
#define INET_ADDR       c_inet_addr
#define inet_aton       c_inet_aton
#define inet_ntoa       c_inet_ntoa
#define inet_ntoa_r     c_inet_ntoa_r
#define inet_pton       c_inet_pton
#define inet_ntop       c_inet_ntop
#else
#define SOCKET           socket
#define BIND             bind
#define CONNECT          connect
#define LISTEN           listen
#define ACCEPT           accept
#define SEND             send
#define SENDTO           sendto
#define SENDMSG          sendmsg
#define RECV             recv
#define RECVFROM         recvfrom
#define RECVMSG          recvmsg
#define getsockname      getsockname
#define getpeername      getpeername
#define SETSOCKOPT       setsockopt
#define GETSOCKOPT       getsockopt
#define IOCTLSOCKET      ioctlsocket
#define FCNTL            fcntl
#define SHUTDOWN         shutdown
#define CLOSESOCKET      closesocket
#define SELECT           select
#define GETHOSTNAME      gethostname
#define GETHOSTBYNAME    gethostbyname
#define gethostbyname_r  gethostbyname_r
#define getaddrinfo      getaddrinfo
#define freeaddrinfo     freeaddrinfo
#define getnameinfo      getnameinfo
#define INET_ADDR        inet_addr
#define inet_aton        inet_aton
#define inet_ntoa        inet_ntoa
#define inet_ntoa_r      inet_ntoa_r
#define inet_pton        inet_pton
#define inet_ntop        inet_ntop
#endif

#endif
