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
ifeq ($(HOST_OS),Darwin)
UPLOAD_PORT ?= /dev/cu.usbmodem0000000000001
else
UPLOAD_PORT ?= /dev/ttyUSB0
endif

ARCHIVE = $(BIN_DIR)/$(NAME).xsa

XSBUG_HOST ?= localhost
XSBUG_PORT ?= 5002

ifeq ($(DEBUG),1)
	LAUNCH = debug
else
	LAUNCH = release
endif

all: $(LAUNCH)
	
debug: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && serial2xsbug $(UPLOAD_PORT) 921600 8N1 -install $(ARCHIVE)

release: $(ARCHIVE)
	$(shell pkill serial2xsbug)
	export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && serial2xsbug $(UPLOAD_PORT) 921600 8N1 -install $(ARCHIVE)

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	xsl -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
