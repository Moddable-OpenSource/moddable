#
# Copyright (c) 2016-2026 Moddable Tech, Inc.
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
	VERS = $(shell sw_vers -productVersion | cut -f1 -d.)
	ifeq ($(shell test $(VERS) -gt 10; echo $$?), 0)
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

KILL_SERIAL2XSBUG = $(shell pkill serial2xsbug)
SERIAL_CMD = serial2xsbug $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)
DO_MOD_UPLOAD = export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && $(SERIAL_CMD)

ifeq ($(DEBUG),1)
	ifeq ("$(XSBUG_LAUNCH)","app")
		ifeq ($(HOST_OS),Darwin)
			START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		else
			START_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
		endif
	endif

	ifeq ("$(XSBUG_LAUNCH)","log")
		START_XSBUG =
		DO_MOD_UPLOAD = export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && cd $(MODDABLE)/tools/xsbug-log && node xsbug-log $(SERIAL_CMD)
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

build: $(ARCHIVE)
	@echo "# Target built: $(BIN_DIR)/$(NAME).xsa"

debugURL: $(ARCHIVE)
	@echo "# curl "$(NAME)".xsa "$(URL)
	curl -X PUT $(URL) -H "Content-Type: application/octet-stream" -H "Expect:" --data-binary '@$(ARCHIVE)' --retry 2 --retry-delay 30

releaseURL: $(ARCHIVE)
	@echo "# curl "$(NAME)".xsa "$(URL)
	curl -X PUT $(URL) -H "Content-Type: application/octet-stream" -H "Expect:" --data-binary '@$(ARCHIVE)' --retry 2 --retry-delay 30

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	xsl -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) $(DATA) $(MODULES) $(RESOURCES)

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
