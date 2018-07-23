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
#

% : %.c
%.o : %.c

GOAL ?= debug
NAME = simulator
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

BUILD_DIR ?= $(realpath ../..)

BIN_DIR = $(BUILD_DIR)/bin/mac/$(GOAL)
TMP_DIR = $(BUILD_DIR)/tmp/mac/$(GOAL)/$(NAME)

COMMODETTO = $(MODDABLE)/modules/commodetto
SIMULATOR = $(MODDABLE)/build/simulator

APP_DIR = $(BIN_DIR)/Screen\ Test.app/Contents/MacOS
RES_DIR = $(BIN_DIR)/Screen\ Test.app/Contents/Resources

ICNS = $(SIMULATOR)/mac/main.icns
PLIST =  $(SIMULATOR)/mac/main.plist
SCREENS = $(wildcard $(SIMULATOR)/screens/*)

MACOS_ARCH ?= -arch i386
MACOS_VERSION_MIN ?= -mmacosx-version-min=10.7

C_OPTIONS = $(MACOS_ARCH) $(MACOS_VERSION_MIN) -fasm-blocks -fno-common -fvisibility=hidden
C_OPTIONS += -I$(COMMODETTO) -I$(SIMULATOR)
ifneq ("x$(SDKROOT)", "x")
	C_OPTIONS += -isysroot $(SDKROOT)
endif
ifeq ($(GOAL),debug)
	C_OPTIONS += -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_OPTIONS += -O3
endif

LIBRARIES = -framework CoreFoundation -framework CoreServices -framework Cocoa

LINK_OPTIONS = $(MACOS_VERSION_MIN) $(MACOS_ARCH) -ObjC
ifneq ("x$(SDKROOT)", "x")
	LINK_OPTIONS += -isysroot $(SDKROOT)
endif

build: $(TMP_DIR) $(APP_DIR) $(APP_DIR)/main $(RES_DIR)/English.lproj/main.nib

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
	
$(APP_DIR): $(ICNS) $(PLIST) $(SCREENS) $(SIMULATOR)/mac/fingerprint.png
	mkdir -p $(APP_DIR)
	mkdir -p $(RES_DIR)/English.lproj
	mkdir -p $(RES_DIR)/screens
	cp -rf $(ICNS) $(RES_DIR)
	cp -rf $(SIMULATOR)/screens/* $(RES_DIR)/screens
	cp -rf $(SIMULATOR)/mac/fingerprint.png $(RES_DIR)
	cp -rf $(PLIST) $(APP_DIR)/../Info.plist
	echo APPLTINY > $(APP_DIR)/../PkgInfo

$(APP_DIR)/main: $(TMP_DIR)/main.o
	@echo "#" $(NAME) $(GOAL) ": cc simulator"
	$(CC) $(LINK_OPTIONS) $(LIBRARIES) $(TMP_DIR)/main.o -o '$@'

$(TMP_DIR)/main.o: $(SIMULATOR)/mac/main.m $(SIMULATOR)/screen.h
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_OPTIONS) -c -o $@

$(RES_DIR)/English.lproj/main.nib: $(SIMULATOR)/mac/main.xib
	ibtool $(SIMULATOR)/mac/main.xib --compile '$@'
	
clean:
	rm -rf $(BUILD_DIR)/bin/mac/debug/$(NAME).app
	rm -rf $(BUILD_DIR)/bin/mac/release/$(NAME).app
	rm -rf $(BUILD_DIR)/tmp/mac/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/mac/release/$(NAME)
		
	
	
	
	
	
	
	
