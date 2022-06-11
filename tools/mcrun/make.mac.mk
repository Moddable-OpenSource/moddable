#
# Copyright (c) 2016-2020 Moddable Tech, Inc.
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

ifeq ($(DEBUG),1)
	START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
	KILL_SERIAL2XSBUG = $(shell pkill serial2xsbug)
else
	START_XSBUG =
	KILL_SERIAL2XSBUG =
endif

BUILDCLUT = $(BUILD_DIR)/bin/mac/release/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/mac/release/compressbmf
IMAGE2CS = $(BUILD_DIR)/bin/mac/release/image2cs
MCLOCAL = $(BUILD_DIR)/bin/mac/release/mclocal
MCREZ = $(BUILD_DIR)/bin/mac/release/mcrez
PNG2BMP = $(BUILD_DIR)/bin/mac/release/png2bmp
RLE4ENCODE = $(BUILD_DIR)/bin/mac/release/rle4encode
WAV2MAUD = $(BUILD_DIR)/bin/mac/release/wav2maud
XSC = $(MODDABLE)/build/bin/mac/release/xsc
XSL = $(MODDABLE)/build/bin/mac/release/xsl

.PHONY: all	

all: $(BIN_DIR)/mc.xsa
	$(KILL_SERIAL2XSBUG) 
	$(START_XSBUG) 
	open -a $(SIMULATOR) $(BIN_DIR)/mc.xsa

build: $(BIN_DIR)/mc.xsa

$(BIN_DIR)/mc.xsa: $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl mc.xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

