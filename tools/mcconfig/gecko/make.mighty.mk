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

# Mighty Gecko radio test (BRD4162A)
SDK_BASE = $(BASE)/sdks/gecko_sdk_suite/v2.1
HWPART = EFR32MG12P332F1024GL125
HWKIT = $(SDK_BASE)/hardware/kit/EFR32MG12_BRD4162A/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFR32MG12P/Include
HWCPU = cortex-m4
DEV_C_FLAGS =
HW_DEBUG_OPT = -O0
HW_OPT = -O3


ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/gecko/mighty/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/gecko/mighty/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/gecko/mighty/release/lib
	endif
endif

C_DEFINES += -DuseRTCC=1 -DMIGHTY_GECKO=1

include $(MODDABLE)/tools/mcconfig/gecko/make.gecko_common.mk

