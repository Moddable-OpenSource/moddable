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

% : %.c
%.o : %.c

GOAL ?= debug
NAME = xst
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

XS_DIR ?= $(realpath ../..)
BUILD_DIR ?= $(realpath ../../../build)

BIN_DIR = $(BUILD_DIR)/bin/lin/$(GOAL)
INC_DIR = $(XS_DIR)/includes
PLT_DIR = $(XS_DIR)/platforms
SRC_DIR = $(XS_DIR)/sources
TLS_DIR = $(XS_DIR)/tools
TMP_DIR = $(BUILD_DIR)/tmp/lin/$(GOAL)/$(NAME)

MACOS_ARCH ?= -arch i386
MACOS_VERSION_MIN ?= -mmacosx-version-min=10.7

C_OPTIONS = \
	-fno-common \
	-DINCLUDE_XSPLATFORM \
	-DXSPLATFORM=\"xst.h\" \
	-DmxDebug=1 \
	-DmxNoConsole=1 \
	-DmxParse=1 \
	-DmxRun=1 \
	-DmxSloppy=1 \
	-I$(INC_DIR) \
	-I$(PLT_DIR) \
	-I$(SRC_DIR) \
	-I$(TLS_DIR) \
	-I$(TLS_DIR)/yaml \
	-I$(TMP_DIR)
ifeq ($(GOAL),debug)
	C_OPTIONS += -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_OPTIONS += -O3
endif

LIBRARIES = -ldl -lm -lpthread

LINK_OPTIONS = -rdynamic

OBJECTS = \
	$(TMP_DIR)/xsAll.o \
	$(TMP_DIR)/xsAPI.o \
	$(TMP_DIR)/xsArguments.o \
	$(TMP_DIR)/xsArray.o \
	$(TMP_DIR)/xsAtomics.o \
	$(TMP_DIR)/xsBoolean.o \
	$(TMP_DIR)/xsCode.o \
	$(TMP_DIR)/xsCommon.o \
	$(TMP_DIR)/xsDataView.o \
	$(TMP_DIR)/xsDate.o \
	$(TMP_DIR)/xsDebug.o \
	$(TMP_DIR)/xsDefaults.o \
	$(TMP_DIR)/xsError.o \
	$(TMP_DIR)/xsFunction.o \
	$(TMP_DIR)/xsGenerator.o \
	$(TMP_DIR)/xsGlobal.o \
	$(TMP_DIR)/xsJSON.o \
	$(TMP_DIR)/xsLexical.o \
	$(TMP_DIR)/xsMapSet.o \
	$(TMP_DIR)/xsMarshall.o \
	$(TMP_DIR)/xsMath.o \
	$(TMP_DIR)/xsMemory.o \
	$(TMP_DIR)/xsModule.o \
	$(TMP_DIR)/xsNumber.o \
	$(TMP_DIR)/xsObject.o \
	$(TMP_DIR)/xsPlatforms.o \
	$(TMP_DIR)/xsProfile.o \
	$(TMP_DIR)/xsPromise.o \
	$(TMP_DIR)/xsProperty.o \
	$(TMP_DIR)/xsProxy.o \
	$(TMP_DIR)/xsRegExp.o \
	$(TMP_DIR)/xsRun.o \
	$(TMP_DIR)/xsScope.o \
	$(TMP_DIR)/xsScript.o \
	$(TMP_DIR)/xsSourceMap.o \
	$(TMP_DIR)/xsString.o \
	$(TMP_DIR)/xsSymbol.o \
	$(TMP_DIR)/xsSyntaxical.o \
	$(TMP_DIR)/xsTree.o \
	$(TMP_DIR)/xsType.o \
	$(TMP_DIR)/xsdtoa.o \
	$(TMP_DIR)/xsre.o \
	$(TMP_DIR)/api.o \
	$(TMP_DIR)/dumper.o \
	$(TMP_DIR)/emitter.o \
	$(TMP_DIR)/loader.o \
	$(TMP_DIR)/parser.o \
	$(TMP_DIR)/reader.o \
	$(TMP_DIR)/scanner.o \
	$(TMP_DIR)/writer.o \
	$(TMP_DIR)/xst.o

VPATH += $(SRC_DIR) $(TLS_DIR) $(TLS_DIR)/yaml

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(NAME): $(OBJECTS)
	@echo "#" $(NAME) $(GOAL) ": cc" $(@F)
	$(CC) $(LINK_OPTIONS) $(OBJECTS) $(LIBRARIES) -o $@
	
$(OBJECTS): $(TLS_DIR)/xst.h
$(OBJECTS): $(PLT_DIR)/xsPlatform.h
$(OBJECTS): $(SRC_DIR)/xsCommon.h
$(OBJECTS): $(SRC_DIR)/xsAll.h
$(OBJECTS): $(SRC_DIR)/xsScript.h
$(TMP_DIR)/%.o: %.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_OPTIONS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)/bin/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/bin/lin/release/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/lin/release/$(NAME)
