#
# Copyright (c) 2016-2021  Moddable Tech, Inc.
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

CC = emcc
OPT = wasm-opt

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources

XS_HEADERS = \
	$(XS_DIR)/platforms/wasm_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)/wasm_xs.c.o \
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
	-DXSPLATFORM=\"wasm_xs.h\" \
	-DmxRun=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)

C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(TMP_DIR),-I$(dir))

C_FLAGS = -c
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

# LINK_OPTIONS = -arch i386 -dynamiclib -flat_namespace -undefined suppress -Wl,-exported_symbol,_fxScreenLaunch -Wl,-dead_strip
LINK_OPTIONS = \
	-s ENVIRONMENT=web\
	-s ALLOW_MEMORY_GROWTH=1\
	-s MODULARIZE=1\
	-s EXPORT_ES6=1\
	-s USE_ES6_IMPORT_META=0\
	-s EXPORT_NAME=mc\
	-s INVOKE_RUN=0\
	-s FORCE_FILESYSTEM=1\
	-s "EXPORTED_FUNCTIONS=['_fxMainIdle', '_fxMainLaunch', '_fxMainQuit', '_fxMainTouch']"

LINK_LIBRARIES = -ldl -lm

ifeq ($(HOST_OS),Darwin)
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac
else
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin
endif

BUILDCLUT = $(MODDABLE_TOOLS_DIR)/release/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/release/compressbmf
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/release/image2cs
MCLOCAL = $(MODDABLE_TOOLS_DIR)/debug/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/release/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/debug/png2bmp
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/release/rle4encode
WAV2MAUD = $(MODDABLE_TOOLS_DIR)/release/wav2maud
XSC = $(MODDABLE_TOOLS_DIR)/release/xsc
XSID = $(MODDABLE_TOOLS_DIR)/release/xsid
XSL = $(MODDABLE_TOOLS_DIR)/release/xsl

VPATH += $(XS_DIRECTORIES)

.PHONY: all	
	
all: $(LIB_DIR) $(BIN_DIR)/mc.js $(BIN_DIR)/index.html
build: all

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
$(BIN_DIR)/index.html: 	$(BUILD_DIR)/makefiles/wasm/index.html
	@echo "# cp index.html"
	cp $< $@
	
$(BIN_DIR)/mc.js: $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) $(TMP_DIR)/mc.main.c.o
	@echo "# cc mc.js"
	$(CC) $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -o $@
	@echo "# wasm-opt"
	$(OPT) -O2 $(BIN_DIR)/mc.wasm -o $(BIN_DIR)/mc.wasm

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
	
$(TMP_DIR)/mc.main.c.o: $(BUILD_DIR)/makefiles/wasm/main.c $(HEADERS)
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
