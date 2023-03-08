// Generated by write_buildconfig_header.py
// From "//third_party/connectedhomeip/src/inet:gen_inet_buildconfig"

#ifndef INET_INETBUILDCONFIG_H_
#define INET_INETBUILDCONFIG_H_

#define INET_CONFIG_TEST 1
#define INET_CONFIG_ENABLE_IPV4 1
#define INET_CONFIG_ENABLE_TCP_ENDPOINT 1
#define INET_CONFIG_ENABLE_UDP_ENDPOINT 1
#define HAVE_LWIP_RAW_BIND_NETIF 1
#define INET_PLATFORM_CONFIG_INCLUDE "platform/ESP32/InetPlatformConfig.h"
#define INET_TCP_END_POINT_IMPL_CONFIG_FILE "inet/TCPEndPointImplLwIP.h"
#define INET_UDP_END_POINT_IMPL_CONFIG_FILE "inet/UDPEndPointImplLwIP.h"

#endif  // INET_INETBUILDCONFIG_H_
