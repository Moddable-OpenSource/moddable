#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

BASE = /Applications/Simplicity?Studio.app/Contents/Eclipse/developer

# Mighty Gecko radio test (BRD4162A)
SDK_BASE = $(BASE)/sdks/gecko_sdk_suite/v1.0
HWPART = EFR32MG12P332F1024GL125
HWKIT = $(SDK_BASE)/hardware/kit/EFR32MG12_BRD4162A/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFR32MG12P/Include
HWCPU = cortex-m4
DEV_C_FLAGS =
HW_DEBUG_OPT = -O0
HW_OPT = -O3

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/gecko_mighty/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_mighty/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_mighty/release/lib
	endif
endif

include $(MODDABLE)/tools/mcconfig/manifest.gecko_common.mk

