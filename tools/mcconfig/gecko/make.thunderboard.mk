#
# Copyright (c) 2016-2018  Moddable Tech, Inc.
#
#   This file is part of the Moddable SDK Tools.
#
#   The Moddable SDK Tools is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   The Moddable SDK Tools is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
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

