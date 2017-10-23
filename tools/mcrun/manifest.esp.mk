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

BASE = ~/esp
UPLOAD_PORT ?= /dev/cu.SLAB_USBtoUART

ESPTOOL = $(BASE)/esptool/esptool
SERIAL2XSBUG = $(MODDABLE)/build/bin/mac/release/serial2xsbug
XSC = $(MODDABLE)/build/bin/mac/release/xsc
XSL = $(MODDABLE)/build/bin/mac/debug/xsl

ARCHIVE =  $(TMP_DIR)/$(NAME).xsa
CODE =  $(TMP_DIR)/$(NAME).xsb
DATA =  $(TMP_DIR)/$(NAME).dat

all: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	$(ESPTOOL) -cd nodemcu -cb 921600 -cp $(UPLOAD_PORT) -ca 0x100000 -cf $(ARCHIVE)
	$(ESPTOOL) -cd nodemcu -cb 115200 -cp $(UPLOAD_PORT) -cr
	$(SERIAL2XSBUG) $(UPLOAD_PORT) 115200 8N1

$(ARCHIVE): $(CODE) $(DATA)
	@echo "# xsl "$(NAME)".xsa"
	$(XSL) -a -o $(TMP_DIR) -r $(NAME) $(CODE) $(CODE) $(DATA) $(DATA)

$(DATA): 
	echo "this is a test" > $(DATA)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

