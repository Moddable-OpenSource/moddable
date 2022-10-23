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
# This file incorporates work covered by the following copyright and  
# permission notice:  
#
#       Copyright (C) 2010-2016 Marvell International Ltd.
#       Copyright (C) 2002-2010 Kinoma, Inc.
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#

% : %.c
%.o : %.c

GOAL ?= debug
NAME = xsc
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

XS_DIR ?= $(realpath ../..)
BUILD_DIR ?= $(realpath ../../../build)

BIN_DIR = $(BUILD_DIR)/bin/mac/$(GOAL)
INC_DIR = $(XS_DIR)/includes
PLT_DIR = $(XS_DIR)/platforms
SRC_DIR = $(XS_DIR)/sources
TLS_DIR = $(XS_DIR)/tools
TMP_DIR = $(BUILD_DIR)/tmp/mac/$(GOAL)/$(NAME)

# MACOS_ARCH ?= -arch i386
MACOS_ARCH ?=
MACOS_VERSION_MIN ?= -mmacosx-version-min=10.7

C_OPTIONS =\
	-fno-common\
	$(MACOS_ARCH)\
	$(MACOS_VERSION_MIN)\
	-I$(INC_DIR)\
	-I$(PLT_DIR) \
	-I$(SRC_DIR)\
	-I$(TLS_DIR)\
	-I$(TMP_DIR)\
	-DmxCompile=1\
	-DmxParse=1
ifneq ("x$(SDKROOT)", "x")
	C_OPTIONS += -isysroot $(SDKROOT)
endif
ifeq ($(GOAL),debug)
	C_OPTIONS += -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_OPTIONS += -O3
endif

LIBRARIES = -framework CoreServices

LINK_OPTIONS = $(MACOS_VERSION_MIN) $(MACOS_ARCH)
ifneq ("x$(SDKROOT)", "x")
	LINK_OPTIONS += -isysroot $(SDKROOT)
endif

OBJECTS = \
	$(TMP_DIR)/xsBigInt.o \
	$(TMP_DIR)/xsCode.o \
	$(TMP_DIR)/xsCommon.o \
	$(TMP_DIR)/xsdtoa.o \
	$(TMP_DIR)/xsLexical.o \
	$(TMP_DIR)/xsre.o \
	$(TMP_DIR)/xsScope.o \
	$(TMP_DIR)/xsScript.o \
	$(TMP_DIR)/xsSourceMap.o \
	$(TMP_DIR)/xsSyntaxical.o \
	$(TMP_DIR)/xsTree.o \
	$(TMP_DIR)/xsc.o

VPATH += $(SRC_DIR) $(TLS_DIR)

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(NAME): $(OBJECTS)
	@echo "#" $(NAME) $(GOAL) ": cc" $(@F)
	$(CC) $(LINK_OPTIONS) $(LIBRARIES) $(OBJECTS) -o $@

$(OBJECTS): $(PLT_DIR)/xsPlatform.h
$(OBJECTS): $(SRC_DIR)/xsCommon.h
$(OBJECTS): $(SRC_DIR)/xsScript.h

$(TMP_DIR)/%.o: %.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_OPTIONS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)/bin/mac/debug/$(NAME)
	rm -rf $(BUILD_DIR)/bin/mac/release/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/mac/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/mac/release/$(NAME)

