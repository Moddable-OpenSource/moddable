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
NAME = xsl
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

CC = emcc
XS_DIR ?= $(realpath ../..)
BUILD_DIR ?= $(realpath ../../../build)

# TODO: This needs to be platform independend
# Ensure that tools are first build for 
# host platform
# In ../mac
# 	make -f xsl.mk
#		make GOAL=release -f xsl.mk
XSC =  $(BUILD_DIR)/bin/mac/$(GOAL)/xsc

BIN_DIR = $(BUILD_DIR)/bin/wasm/$(GOAL)
INC_DIR = $(XS_DIR)/includes
PLT_DIR = $(XS_DIR)/platforms
SRC_DIR = $(XS_DIR)/sources
TLS_DIR = $(XS_DIR)/tools
TMP_DIR = $(BUILD_DIR)/tmp/wasm/$(GOAL)/$(NAME)

C_OPTIONS =\
	-DINCLUDE_XSPLATFORM \
	-DXSPLATFORM=\"xslOpt.h\" \
	-fno-common\
	-I$(INC_DIR)\
	-I$(PLT_DIR) \
	-I$(SRC_DIR)\
	-I$(TLS_DIR)\
	-I$(TMP_DIR)\
	-DmxLink=1\
	-DmxRun=1
C_OPTIONS +=\
	-DmxNoFunctionLength=1\
	-DmxNoFunctionName=1\
	-DmxHostFunctionPrimitive=1\
	-DmxFewGlobalsTable=1
ifeq ($(GOAL),debug)
	C_OPTIONS += -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_OPTIONS += -Oz
endif

LIBRARIES = -ldl -lm

LINK_OPTIONS =\
	-s ENVIRONMENT=worker\
	-s MODULARIZE=1\
	-s EXPORT_ES6=1\
	-s EXPORT_NAME=$(NAME)\
	-s INVOKE_RUN=0\
	-s FORCE_FILESYSTEM=1\
	-s ERROR_ON_UNDEFINED_SYMBOLS=0\
	-s "EXTRA_EXPORTED_RUNTIME_METHODS=['FS', 'ALLOC_NORMAL','ALLOC_STACK','ALLOC_DYNAMIC','ALLOC_NONE']"
ifeq ($(GOAL),release)
	LINK_OPTIONS += -Oz --closure 0
endif

OBJECTS = \
	$(TMP_DIR)/xsAll.o \
	$(TMP_DIR)/xsAPI.o \
	$(TMP_DIR)/xsArguments.o \
	$(TMP_DIR)/xsArray.o \
	$(TMP_DIR)/xsAtomics.o \
	$(TMP_DIR)/xsBigInt.o \
	$(TMP_DIR)/xsBoolean.o \
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
	$(TMP_DIR)/xsString.o \
	$(TMP_DIR)/xsSymbol.o \
	$(TMP_DIR)/xsType.o \
	$(TMP_DIR)/xsdtoa.o \
	$(TMP_DIR)/xsre.o \
	$(TMP_DIR)/xslBase.o \
	$(TMP_DIR)/xslOpt.o \
	$(TMP_DIR)/xslSlot.o \
	$(TMP_DIR)/xslStrip.o \
	$(TMP_DIR)/xsl.o

VPATH += $(SRC_DIR) $(TLS_DIR)

build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME)

$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(NAME): $(TMP_DIR)/xslOpt.xs.o $(OBJECTS)
	@echo "#" $(NAME) $(GOAL) ":" $(CC) $(@F)
	$(CC) -s ALLOW_MEMORY_GROWTH=1 $(LINK_OPTIONS) $(TMP_DIR)/xslOpt.xs.o $(OBJECTS) $(LIBRARIES) -o $@.js
	
$(OBJECTS): $(PLT_DIR)/xsPlatform.h
$(OBJECTS): $(SRC_DIR)/xsCommon.h
$(OBJECTS): $(SRC_DIR)/xsAll.h
$(OBJECTS): $(TLS_DIR)/xsl.h
$(OBJECTS): $(TLS_DIR)/xslOpt.h
$(TMP_DIR)/xslOpt.o: $(TMP_DIR)/xslOpt.xs.c

$(TMP_DIR)/%.o: %.c
	@echo "#" $(NAME) $(GOAL) ":" $(CC) $(<F)
	$(CC) -s ALLOW_MEMORY_GROWTH=1 $< $(C_OPTIONS) -c -o $@
	
$(TMP_DIR)/xslOpt.xs.o:	 $(TMP_DIR)/xslOpt.xs.c $(PLT_DIR)/xsPlatform.h $(SRC_DIR)/xsCommon.h $(SRC_DIR)/xsAll.h $(TLS_DIR)/xsl.h $(TLS_DIR)/xslOpt.h
	@echo "#" $(NAME) $(GOAL) ":" $(CC) $(<F)
	$(CC) -s ALLOW_MEMORY_GROWTH=1 $< $(C_OPTIONS) -c -o $@

$(TMP_DIR)/xslOpt.xs.c: $(TLS_DIR)/xslOpt.js
	$(XSC) $< -c -d -o $(TMP_DIR) -p 

clean:
	rm -rf $(BUILD_DIR)/bin/wasm/debug/$(NAME).{js,wasm}
	rm -rf $(BUILD_DIR)/bin/wasm/release/$(NAME).{js,wasm}
	rm -rf $(BUILD_DIR)/tmp/wasm/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/wasm/release/$(NAME)



