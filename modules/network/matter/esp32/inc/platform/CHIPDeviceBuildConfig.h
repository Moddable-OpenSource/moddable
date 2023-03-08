// Generated by write_buildconfig_header.py
// From "//third_party/connectedhomeip/src/platform:gen_platform_buildconfig"

#ifndef PLATFORM_CHIPDEVICEBUILDCONFIG_H_
#define PLATFORM_CHIPDEVICEBUILDCONFIG_H_

#define CHIP_DEVICE_CONFIG_ENABLE_WPA 1
#define CHIP_ENABLE_OPENTHREAD 0
#define CHIP_DEVICE_CONFIG_THREAD_FTD 0
#define CHIP_STACK_LOCK_TRACKING_ENABLED 1
#define CHIP_STACK_LOCK_TRACKING_ERROR_FATAL 1
#define CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING 0
#define CHIP_DEVICE_CONFIG_RUN_AS_ROOT 1
#define CHIP_DISABLE_PLATFORM_KVS 0
#define CHIP_DEVICE_PLATFORM_CONFIG_INCLUDE "platform/ESP32/CHIPDevicePlatformConfig.h"
#define CHIP_USE_TRANSITIONAL_COMMISSIONABLE_DATA_PROVIDER 1
#define CHIP_DEVICE_LAYER_TARGET_ESP32 1
#ifdef ESP32
    #undef ESP32
#endif
#define CHIP_DEVICE_LAYER_TARGET ESP32

#endif  // PLATFORM_CHIPDEVICEBUILDCONFIG_H_
