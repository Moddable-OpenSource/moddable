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

START_SIMULATOR = export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && open -a $(SIMULATOR) $(SIMULATORS) $(BIN_DIR)/mc.so

ifeq ($(DEBUG),1)
	ifeq ($(XSBUG_LOG),1)
		START_XSBUG = 
		START_SIMULATOR = export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && cd $(MODDABLE)/tools/xsbug-log && node xsbug-log open -a $(SIMULATOR) $(SIMULATORS) $(BIN_DIR)/mc.so
	else
		START_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
	endif	
	KILL_SERIAL2XSBUG = $(shell pkill serial2xsbug)
else
	START_XSBUG =
	KILL_SERIAL2XSBUG =
endif

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources

XS_HEADERS = \
	$(XS_DIR)/platforms/mac_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)/mac_xs.c.o \
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
	$(LIB_DIR)/xsPlatforms.c.o \
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
	$(LIB_DIR)/xsmc.c.o \
	$(LIB_DIR)/xsre.c.o

HEADERS += $(XS_HEADERS)

C_DEFINES = \
	-DXS_ARCHIVE=1 \
	-DINCLUDE_XSPLATFORM=1 \
	-DXSPLATFORM=\"mac_xs.h\" \
	-DmxRun=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif

C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(TMP_DIR),-I$(dir))

MACOS_ARCH ?=
# C_FLAGS = -c -arch i386
C_FLAGS = -c $(MACOS_ARCH)
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

# LINK_OPTIONS = -arch i386 -dynamiclib -flat_namespace -undefined suppress -Wl,-exported_symbol,_fxScreenLaunch -Wl,-dead_strip
LINK_OPTIONS = -dynamiclib -flat_namespace -undefined suppress -Wl,-exported_symbol,_fxScreenLaunch -Wl,-dead_strip -lobjc $(MACOS_ARCH)

BUILDCLUT = $(BUILD_DIR)/bin/mac/release/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/mac/release/compressbmf
IMAGE2CS = $(BUILD_DIR)/bin/mac/release/image2cs
MCLOCAL = $(BUILD_DIR)/bin/mac/debug/mclocal
MCREZ = $(BUILD_DIR)/bin/mac/release/mcrez
PNG2BMP = $(BUILD_DIR)/bin/mac/debug/png2bmp
RLE4ENCODE = $(BUILD_DIR)/bin/mac/release/rle4encode
WAV2MAUD = $(BUILD_DIR)/bin/mac/release/wav2maud
XSC = $(BUILD_DIR)/bin/mac/release/xsc
XSID = $(BUILD_DIR)/bin/mac/release/xsid
XSL = $(BUILD_DIR)/bin/mac/debug/xsl

VPATH += $(XS_DIRECTORIES)

.PHONY: all	

XSBUG_HOST ?= localhost
XSBUG_PORT ?= 5002
	
all: precursor
	$(KILL_SERIAL2XSBUG) 
	$(START_XSBUG)
	$(START_SIMULATOR)

precursor: $(LIB_DIR) $(BIN_DIR)/mc.so

clean:
	echo "# Clean project"
	-rm -rf $(BIN_DIR) 2>/dev/null
	-rm -rf $(TMP_DIR) 2>/dev/null

build: precursor

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
$(BIN_DIR)/mc.so: $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld mc.so"
	$(CC) $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -o $@
#	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -Wl,-map,map.txt -v -o $@ 

$(XS_OBJECTS) : $(XS_HEADERS)
$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c.o: $(TMP_DIR)/mc.xs.c $(HEADERS)
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c.o: $(TMP_DIR)/mc.resources.c $(HEADERS)
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -r mc.resources.c
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
