#
# Copyright (c) 2016-2017  Moddable Tech, Inc.
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

PROJECT_NAME := xs_esp32

ifeq ("$(OS)","Windows_NT")
BUILD_DIR_BASE := $(realpath $(MODDABLE)/build/tmp/esp32/idf)
else
BUILD_DIR_BASE := $(MODDABLE)/build/tmp/esp32/idf
endif


# COMPONENTS=freertos esp32 newlib esptool_py tcpip_adapter lwip main soc ethernet log driver vfs heap nvs_flash spi_flash bootloader_support app_trace xtensa-debug-module mbedtls wpa_supplicant micro-ecc

CFLAGS += -Wno-unused-value

include $(IDF_PATH)/make/project.mk

