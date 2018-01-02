#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

BASE = /Applications/Simplicity?Studio.app/Contents/Eclipse/developer
# Giant Gecko starter kit
SDK_BASE = $(BASE)/sdks/exx32/v5.0.0.0
HWKIT = $(SDK_BASE)/hardware/kit/EFM32GG_STK3700/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFM32GG/Include 
HWPART = EFM32GG990F1024
HWCPU = cortex-m3
HW_DEBUG_OPT = -O0
HW_OPT = -O3
DEV_C_FLAGS =

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/gecko_giant/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_giant/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_giant/release/lib
	endif
endif

C_DEFINES += -DuseBURTC=1

include $(MODDABLE)/tools/mcconfig/gecko/make.gecko_common.mk

