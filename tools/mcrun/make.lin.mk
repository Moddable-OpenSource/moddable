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

BUILDCLUT = $(BUILD_DIR)/bin/lin/release/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/lin/release/compressbmf
IMAGE2CS = $(BUILD_DIR)/bin/lin/release/image2cs
MCLOCAL = $(BUILD_DIR)/bin/lin/release/mclocal
MCREZ = $(BUILD_DIR)/bin/lin/release/mcrez
PNG2BMP = $(BUILD_DIR)/bin/lin/release/png2bmp
RLE4ENCODE = $(BUILD_DIR)/bin/lin/release/rle4encode
WAV2MAUD = $(BUILD_DIR)/bin/lin/release/wav2maud
XSC = $(MODDABLE)/build/bin/lin/release/xsc
XSL = $(MODDABLE)/build/bin/lin/release/xsl

.PHONY: all	

all: $(BIN_DIR)/mc.xsa
	$(shell nohup $(SIMULATOR) $(BIN_DIR)/mc.xsa > /dev/null 2>&1 &)

$(BIN_DIR)/mc.xsa: $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl mc.xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

