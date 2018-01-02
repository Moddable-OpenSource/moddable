#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

BASE = /Applications/Simplicity?Studio.app/Contents/Eclipse/developer
# Thunderboard	(BRD4160A)
SDK_BASE = $(BASE)/sdks/gecko_sdk_suite/v1.0
HWKIT = $(SDK_BASE)/hardware/kit/EFR32MG1_BRD4160A/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFR32MG1P/Include
HWPART = EFR32MG1P132F256GM48
HWCPU = cortex-m4
HW_DEBUG_OPT= -O0
HW_OPT = -Os
# DEV_C_FLAGS = -fno-short-enums
DEV_C_FLAGS = 

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/gecko_thunderboard/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_thunderboard/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/gecko_thunderboard/release/lib
	endif
endif

C_DEFINES += -DuseRTCC

include $(MODDABLE)/tools/mcconfig/gecko/make.gecko_common.mk

