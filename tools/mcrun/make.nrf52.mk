#
# Copyright (c) 2016-2023  Moddable Tech, Inc.
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

HOST_OS := $(shell uname)

USE_USB ?= 0

DEBUGGER_SPEED ?= 921600

ifeq ($(HOST_OS),Darwin)
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/release
else
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin/release
endif
BUILDCLUT = $(MODDABLE_TOOLS_DIR)/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/compressbmf
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/image2cs
MCLOCAL = $(MODDABLE_TOOLS_DIR)/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/png2bmp
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/rle4encode
WAV2MAUD = $(MODDABLE_TOOLS_DIR)/wav2maud
SERIAL2XSBUG = $(MODDABLE_TOOLS_DIR)/serial2xsbug
XSC = $(MODDABLE_TOOLS_DIR)/xsc
XSL = $(MODDABLE_TOOLS_DIR)/xsl

SCRIPTS_DIR = $(BUILD_DIR)/devices/nrf52/config

ARCHIVE = $(BIN_DIR)/$(NAME).xsa

ifeq ($(DEBUG),1)
	LAUNCH = debug
else
	LAUNCH = release
endif

#	USE_USB = 1

ifeq ($(USE_USB),1)
	M4_VID ?= beef
	M4_PID ?= cafe
endif

KILL_SERIAL2XBUG = $(shell pkill serial2xsbug)

ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		DO_MOD_UPLOAD = if [[ ! -c "$(UPLOAD_PORT)" ]]; then echo "\#\#\#  No port. Set UPLOAD_PORT" ; exit 1; fi && XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE) -norestart

		START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		ifneq ($(USE_USB),0)
			DO_MOD_UPLOAD = XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) $(SERIAL2XSBUG) $(M4_VID):$(M4_PID) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)
		else
		endif
	else
		START_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
		DO_MOD_UPLOAD = if [ ! -c "$(UPLOAD_PORT)" ]; then echo "\#\#\#  Set UPLOAD_PORT" ; exit 1; fi && $(SERIAL2XSBUG) $(DEBUGGER_PORT) 921600 8N1 -install $(ARCHIVE) -norestart

		ifeq ($(UPLOAD_PORT),)
			ifeq ($(USE_USB),1)
				DO_MOD_UPLOAD = echo a ; $(SERIAL2XSBUG) `$(SCRIPTS_DIR)/findUSBLinux $(M4_VID) $(M4_PID) cdc_acm` 921600 8N1 -install $(ARCHIVE)
			endif	
		endif
	endif
endif


all: $(LAUNCH)
	
debug: $(ARCHIVE)
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(DO_MOD_UPLOAD)

release: $(ARCHIVE)
	$(KILL_SERIAL2XSBUG)
	$(DO_MOD_UPLOAD)

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
