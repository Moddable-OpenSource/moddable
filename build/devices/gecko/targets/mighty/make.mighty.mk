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
PLATFORM_NAME = mighty
SDK_BASE = $(BASE)/sdks/gecko_sdk_suite/v2.2
ifeq ("x$(GECKO_BOARD)", "x")
	GECKO_BOARD=BRD4162A
endif
ifeq ("x$(GECKO_PART)", "x")
	GECKO_PART=EFR32MG12P332F1024GL125
endif
HWPART = $(GECKO_PART)
HWKIT = $(SDK_BASE)/hardware/kit/EFR32MG12_$(GECKO_BOARD)/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFR32MG12P/Include
HWCPU = cortex-m4
DEV_C_FLAGS =
HW_DEBUG_OPT = -O0
HW_OPT = -O3


C_DEFINES += -DuseRTCC=1 -DMIGHTY_GECKO=1

include $(BUILD)/devices/gecko/targets/make.gecko_common.mk
