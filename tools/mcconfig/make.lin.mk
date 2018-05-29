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

PKGCONFIG = $(shell which pkg-config)

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/lin/debug/mc/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/lin/instrument/mc/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/lin/release/mc/lib
	endif
endif

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources

XS_HEADERS = \
	$(XS_DIR)/platforms/lin_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)/lin_xs.c.o \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArray.c.o \
	$(LIB_DIR)/xsAtomics.c.o \
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
	$(LIB_DIR)/xsProfile.c.o \
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
	-DXSPLATFORM=\"lin_xs.h\" \
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

C_FLAGS = -fPIC -shared -c $(shell $(PKGCONFIG) --cflags gio-2.0)
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

LINK_LIBRARIES = -lm -lc $(shell $(PKGCONFIG) --libs gio-2.0)

LINK_OPTIONS = -fPIC -shared -Wl,-Bdynamic\,-Bsymbolic

BUILDCLUT = $(BUILD_DIR)/bin/lin/debug/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/lin/debug/compressbmf
IMAGE2CS = $(BUILD_DIR)/bin/lin/debug/image2cs
MCLOCAL = $(BUILD_DIR)/bin/lin/debug/mclocal
MCREZ = $(BUILD_DIR)/bin/lin/debug/mcrez
PNG2BMP = $(BUILD_DIR)/bin/lin/debug/png2bmp
RLE4ENCODE = $(BUILD_DIR)/bin/lin/debug/rle4encode
WAV2MAUD = $(BUILD_DIR)/bin/lin/debug/wav2maud
XSC = $(BUILD_DIR)/bin/lin/debug/xsc
XSID = $(BUILD_DIR)/bin/lin/debug/xsid
XSL = $(BUILD_DIR)/bin/lin/debug/xsl

VPATH += $(XS_DIRECTORIES)

.PHONY: all	
	
all: $(LIB_DIR) $(BIN_DIR)/mc.so $(DATA)
	$(shell nohup $(SIMULATOR) $(BIN_DIR)/mc.so > /dev/null 2>&1 &)
	
$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
$(BIN_DIR)/mc.so: $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld mc.so"
	$(CC) $(LINK_OPTIONS) $^ $(LINK_LIBRARIES) -o $@

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

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -r mc.resources.c
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
