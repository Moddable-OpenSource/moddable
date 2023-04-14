#
# Copyright (c) 2016-2019  Moddable Tech, Inc.
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
.NOTPARALLEL:

SUBPLATFORM?=

NAME = libxs
XS_DIR = $(MODDABLE)/xs
BUILD_DIR = $(MODDABLE)/build
BIN_DIR = $(STAGING_DIR)/lib

ifneq ($(SUBPLATFORM),)
	FULLPLATFORM = openwrt/$(SUBPLATFORM)
else
	FULLPLATFORM = openwrt
endif

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/release/lib
	endif
endif

XS_OBJECTS = \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArguments.c.o \
	$(LIB_DIR)/xsArray.c.o \
	$(LIB_DIR)/xsAtomics.c.o \
	$(LIB_DIR)/xsBigInt.c.o \
	$(LIB_DIR)/xsBoolean.c.o \
	$(LIB_DIR)/xsCode.c.o \
	$(LIB_DIR)/xsCommon.c.o \
	$(LIB_DIR)/xsDataView.c.o \
	$(LIB_DIR)/xsDate.c.o \
	$(LIB_DIR)/xsDebug.c.o \
	$(LIB_DIR)/xsError.c.o \
	$(LIB_DIR)/xsFunction.c.o \
	$(LIB_DIR)/xsGenerator.c.o \
	$(LIB_DIR)/xsGlobal.c.o \
	$(LIB_DIR)/xsJSON.c.o \
	$(LIB_DIR)/xsLexical.c.o \
	$(LIB_DIR)/xsMapSet.c.o \
	$(LIB_DIR)/xsMarshall.c.o \
	$(LIB_DIR)/xsMath.c.o \
	$(LIB_DIR)/xsMemory.c.o \
	$(LIB_DIR)/xsModule.c.o \
	$(LIB_DIR)/xsNumber.c.o \
	$(LIB_DIR)/xsObject.c.o \
	$(LIB_DIR)/xsPromise.c.o \
	$(LIB_DIR)/xsProperty.c.o \
	$(LIB_DIR)/xsProxy.c.o \
	$(LIB_DIR)/xsRegExp.c.o \
	$(LIB_DIR)/xsRun.c.o \
	$(LIB_DIR)/xsScope.c.o \
	$(LIB_DIR)/xsScript.c.o \
	$(LIB_DIR)/xsSourceMap.c.o \
	$(LIB_DIR)/xsString.c.o \
	$(LIB_DIR)/xsSymbol.c.o \
	$(LIB_DIR)/xsSyntaxical.c.o \
	$(LIB_DIR)/xsTree.c.o \
	$(LIB_DIR)/xsType.c.o \
	$(LIB_DIR)/xsdtoa.c.o \
	$(LIB_DIR)/xsre.c.o \
	$(LIB_DIR)/xsmc.c.o
	
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/platforms/openwrt \
	$(BUILD_DIR)/devices/openwrt

XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsScript.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/openwrt/xsHost.h \
	$(XS_DIR)/platforms/openwrt/xsPlatform.h
	
MODULE_DIRS = \
	$(MODDABLE)/modules/base/timer\
	$(MODDABLE)/modules/base/instrumentation

C_DEFINES += \
	-U__STRICT_ANSI__ \
	-Dopenwrt=1 \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1
ifeq ($(DEBUG),1)
	C_DEFINES += -DDEBUG=1 -DmxDebug=1
endif
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif
C_DEFINES += -DmxNoConsole

C_FLAGS = -c $(CFLAGS) -fPIC
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra \
		-Wno-implicit-function-declaration -Wno-missing-field-initializers -Wno-unused-parameter -Wno-misleading-indentation -Wno-implicit-fallthrough
endif

C_INCLUDES += $(foreach dir,$(XS_DIRS) $(LIB_DIR) $(MODULE_DIRS),-I$(dir))

LINK_LIBRARIES = -lm -lc
LINK_OPTIONS = -shared -Wl,-soname,libxs.so

VPATH += $(XS_DIRS)

.PHONY: all

all: $(LIB_DIR) $(BIN_DIR) $(BIN_DIR)/libxs.so

clean:
	rm -rf $(LIB_DIR)
	rm -f $(BIN_DIR)/libxs.so

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)
	
$(BIN_DIR)/libxs.so: $(XS_OBJECTS)
	@echo "# ld libxs.so"
	$(CC) $(LINK_OPTIONS) $^ $(LINK_LIBRARIES) -o $@
	cp $@ $(PKG_BUILD_DIR)
	
$(XS_OBJECTS): $(XS_HEADERS)

$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif


