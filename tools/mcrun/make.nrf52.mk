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

M4_VID ?= beef
M4_PID ?= cafe

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

ifeq ($(DEBUG),1)
	LAUNCH = debug
else
	LAUNCH = release
endif

ifeq ($(HOST_OS),Darwin)
	INSTALL_ARCHIVE = $(SERIAL2XSBUG) $(M4_VID):$(M4_PID) 921600 8N1 -install $(ARCHIVE)
else
	ifeq ($(DEBUGGER_PORT),)
		INSTALL_ARCHIVE = $(SERIAL2XSBUG) `$(MODDABLE_TOOLS_DIR)/findUSBLinux $(M4_VID) $(M4_PID) cdc_acm` 921600 8N1 -install $(ARCHIVE)
	else
		INSTALL_ARCHIVE = $(SERIAL2XSBUG) $(DEBUGGER_PORT) 921600 8N1 -install $(ARCHIVE)
	endif
endif

all: $(LAUNCH)
	
debug: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	$(INSTALL_ARCHIVE)

release: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	$(INSTALL_ARCHIVE)

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
