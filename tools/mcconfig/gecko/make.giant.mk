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

# Giant Gecko starter kit
PLATFORM_NAME = giant
SDK_BASE = $(BASE)/sdks/exx32/v5.0.0.0
ifeq ("x$(GECKO_BOARD)", "x")
    GECKO_BOARD=STK3700
endif
ifeq ("x$(GECKO_PART)", "x")
    GECKO_PART=EFM32GG990F1024
endif
HWKIT = $(SDK_BASE)/hardware/kit/EFM32GG_$(GECKO_BOARD)/config
HWINC = $(SDK_BASE)/platform/Device/SiliconLabs/EFM32GG/Include 
HWPART = $(GECKO_PART)
HWCPU = cortex-m3
HW_DEBUG_OPT = -O0
HW_OPT = -O3
DEV_C_FLAGS =

C_DEFINES += -DuseBURTC=1 -DGIANT_GECKO=1

include $(MODDABLE)/tools/mcconfig/gecko/make.gecko_common.mk

