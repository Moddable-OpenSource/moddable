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

USE_USB ?= 0
ifeq ($(USE_USB),0)
ifeq ($(UPLOAD_PORT),)
	UPLOAD_PORT ?= $(shell bash -c "$(BUILD_DIR)/devices/esp32/config/idfSerialPort")
endif
endif

URL ?= "~"

DEBUGGER_SPEED ?= 460800

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

#	USE_USB = 1

ifeq ($(USE_USB),1)
	USB_VENDOR_ID ?= beef
	USB_PRODUCT_ID ?= 1cee
else
	USB_VENDOR_ID ?= 303a
	USB_PRODUCT_ID ?= 1001
endif
PROGRAMMING_VID ?= 303a
PROGRAMMING_PID ?= 1001

KILL_SERIAL2XSBUG = $(shell pkill serial2xsbug)
DO_MOD_UPLOAD = if [[ ! -c "$(UPLOAD_PORT)" ]]; then echo "No port." ; exit 1; fi && XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)

ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		ifneq ($(USE_USB),0)
			DO_MOD_UPLOAD = XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(USB_VENDOR_ID):$(USB_PRODUCT_ID) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE) -norestart
		else
		endif
	else
		START_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
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
