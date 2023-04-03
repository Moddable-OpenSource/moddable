#
# Copyright (c) 2016-2023 Moddable Tech, Inc.
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
ifeq ($(HOST_OS),Darwin)
	ifeq ($(findstring _13.,_$(shell sw_vers -productVersion)),_13.)
		UPLOAD_PORT ?= /dev/cu.usbserial-0001
	else ifeq ($(findstring _12.,_$(shell sw_vers -productVersion)),_12.)
		UPLOAD_PORT ?= /dev/cu.usbserial-0001
	else ifeq ($(findstring _11.,_$(shell sw_vers -productVersion)),_11.)
		UPLOAD_PORT ?= /dev/cu.usbserial-0001
	else
		UPLOAD_PORT ?= /dev/cu.SLAB_USBtoUART
	endif
else
	UPLOAD_PORT ?= /dev/ttyUSB0
endif

URL ?= "~"

DEBUGGER_SPEED ?= 921600

XSBUG_HOST ?= localhost
XSBUG_PORT ?= 5002

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

ARCHIVE = $(BIN_DIR)/$(NAME).xsa

ifeq ($(URL),"~")
ifeq ($(DEBUG),1)
	LAUNCH = debug
else
	LAUNCH = release
endif
else
ifeq ($(DEBUG),1)
	LAUNCH = debugURL
else
	LAUNCH = releaseURL
endif
endif

ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
	else
		START_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
	endif
endif

all: $(LAUNCH)
	
debug: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	$(START_XSBUG)
	export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)

release: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)

debugURL: $(ARCHIVE)
	@echo "# curl "$(NAME)".xsa "$(URL)
	curl -X PUT $(URL) -H "Content-Type: application/octet-stream" -H "Expect:" --data-binary '@$(ARCHIVE)' --retry 2 --retry-delay 30

releaseURL: $(ARCHIVE)
	@echo "# curl "$(NAME)".xsa "$(URL)
	curl -X PUT $(URL) -H "Content-Type: application/octet-stream" -H "Expect:" --data-binary '@$(ARCHIVE)' --retry 2 --retry-delay 30

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
