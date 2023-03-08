// Generated by write_buildconfig_header.py
// From "//third_party/connectedhomeip/src/system:gen_system_buildconfig"

#ifndef SYSTEM_SYSTEMBUILDCONFIG_H_
#define SYSTEM_SYSTEMBUILDCONFIG_H_

#define CONFIG_DEVICE_LAYER 1
#define CHIP_SYSTEM_CONFIG_TEST 1
#define CHIP_WITH_NLFAULTINJECTION 1
#define CHIP_SYSTEM_CONFIG_USE_DISPATCH 0
#define CHIP_SYSTEM_CONFIG_USE_LWIP 1
#define CHIP_SYSTEM_CONFIG_USE_OPEN_THREAD_ENDPOINT 0
#define CHIP_SYSTEM_CONFIG_USE_SOCKETS 0
#define CHIP_SYSTEM_CONFIG_USE_NETWORK_FRAMEWORK 0
#define CHIP_SYSTEM_CONFIG_POSIX_LOCKING 0
#define CHIP_SYSTEM_CONFIG_FREERTOS_LOCKING 1
#define CHIP_SYSTEM_CONFIG_MBED_LOCKING 0
#define CHIP_SYSTEM_CONFIG_CMSIS_RTOS_LOCKING 0
#define CHIP_SYSTEM_CONFIG_NO_LOCKING 0
#define CHIP_SYSTEM_CONFIG_PROVIDE_STATISTICS 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CLOCK_SETTIME 1
#define HAVE_GETTIMEOFDAY 0
#define HAVE_SYS_TIME_H 1
#define HAVE_NETINET_ICMP6_H 1
#define HAVE_ICMP6_FILTER 1
#define CONFIG_HAVE_VCBPRINTF 0
#define CONFIG_HAVE_VSNPRINTF_EX 0
#define HAVE_SYS_SOCKET_H 0
#define CHIP_PLATFORM_CONFIG_INCLUDE "platform/ESP32/CHIPPlatformConfig.h"
#define SYSTEM_PLATFORM_CONFIG_INCLUDE "platform/ESP32/SystemPlatformConfig.h"
#define CHIP_SYSTEM_LAYER_IMPL_CONFIG_FILE "system/SystemLayerImplFreeRTOS.h"
#define CHIP_SYSTEM_CONFIG_MULTICAST_HOMING 0 

#endif  // SYSTEM_SYSTEMBUILDCONFIG_H_
