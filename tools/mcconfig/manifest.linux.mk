#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

INC_DIRS = \

SDK = \

SDK_SRC =
SDK_OBJ = 
SDK_DIRS = 

ifeq ($DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/linux/device/debug/lib
else
	ifeq ($INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/linux/device/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/linux/device/release/lib
	endif
endif
	

XS_OBJ = \
    $(LIB_DIR)/xsHost.c.o \
    $(LIB_DIR)/xsPlatform.c.o \
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
    $(LIB_DIR)/xsPromise.c.o \
    $(LIB_DIR)/xsProperty.c.o \
    $(LIB_DIR)/xsProxy.c.o \
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
    $(LIB_DIR)/hspi.c.o \
    $(LIB_DIR)/gpio.c.o \
    $(LIB_DIR)/main.c.o
#    $(LIB_DIR)/xsTimer.c.o \
#    $(LIB_DIR)/devfb.c.o \
#    $(LIB_DIR)/xsRegExp.c.o \
#    $(LIB_DIR)/xsp6.c.o \
#    $(LIB_DIR)/xsSocket.c.o 
XS_DIRS = \
    $(XS_DIR)/includes \
    $(XS_DIR)/../build/devices/linux \
    $(XS_DIR)/sources \
    $(XS_DIR)/sources/linux \
    $(XS_DIR)/../build/devices/linux/spi \
    $(XS_DIR)/../build/devices/linux/gpio \
    $(XS_DIR)/modules/drivers/devfb \
    $(XS_DIR)/modules/base/timer
# 	$(XS_DIR)/others/tool
#    $(XS_DIR)/modules/psocket 


# XS_DIRS = \
# 	$(XS_DIR)/includes \
# 	$(XS_DIR)/sources \
# 	$(XS_DIR)/others/tool \
# 	$(XS_DIR)/others/pcre

C_DEFINES = \
	-DXS_ARCHIVE=1 \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DmxParse=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxNoSuchFunctions=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoFrameBuffer=1 \
	-DkPocoRotation=$(ROTATION)
ifeq ($(INSTRUMENT)",1)
	C_DEFINES = $(C_DEFINES) \
		-DMODINSTRUMENTATION=1
endif

C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(INC_DIRS) $(SDK_DIRS) $(XS_DIRS) $(LIB_DIR) $(TMP_DIR),-I$(dir))
C_FLAGS = -c -fPIC
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

CPP_FLAGS = $(C_FLAGS)

TOOLS_BIN = $(MODDABLE_GNUEABI)/bin
CC  = $(TOOLS_BIN)/$(MODDABLE_ARCH)-gcc
CPP = $(TOOLS_BIN)/$(MODDABLE_ARCH)-g++
LD  = $(CC)
AR  = $(TOOLS_BIN)/$(MODDABLE_ARCH)-ar

LINK_LIBRARIES = -lc -lm

LINK_OPTIONS = -fPIC --sysroot=$(MODDABLE_SYSROOT) -Wl,-Bdynamic\,-Bsymbolic -Wl,-rpath,$(MODDABLE_SYSROOT)/usr/lib,-z,muldefs -Wl,-rpath,.,-rpath,'$ORIGIN/lib' -L$(MODDABLE_SYSROOT)/usr/lib -L$(MODDABLE_SYSROOT)/usr/lib/$(MODDABLE_ARCH)

BUILDCLUT = $(BUILD_DIR)/bin/linux/debug/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/linux/debug/compressbmf
RLE4ENCODE = $(BUILD_DIR)/bin/linux/debug/rle4encode
MCLOCAL = $(BUILD_DIR)/bin/linux/debug/mclocal
MCREZ = $(BUILD_DIR)/bin/linux/debug/mcrez
PNG2BMP = $(BUILD_DIR)/bin/linux/debug/png2bmp
IMAGE2CS = $(BUILD_DIR)/bin/mac/debug/image2cs
XSC = $(BUILD_DIR)/bin/linux/debug/xsc
XSID = $(BUILD_DIR)/bin/mac/debug/xsid
XSL = $(BUILD_DIR)/bin/linux/debug/xsl

VPATH += $(SDK_DIRS) $(XS_DIRS)

.PHONY: all	

all: $(LIB_DIR) $(DATA) $(BIN_DIR)/main
#all: $(BIN_DIR)/mc.so $(DATA) $(BIN_DIR)/main
#	open -a $(BIN_DIR)/../../../debug/Screen\ Test.app $(BIN_DIR)/mc.so

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(BIN_DIR)/main: $(SDK_OBJ) $(XS_OBJ) $(TMP_DIR)/mc.xs.o $(TMP_DIR)/mc.resources.o $(OBJECTS)
	@echo "# ld main"
	$(CC) $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -o $@

# $(BIN_DIR)/mc.so: $(TMP_DIR)/mc.xs.o $(TMP_DIR)/mc.resources.o $(OBJECTS) 
# 	@echo "# ld mc.so"
# 	$(CC) $(LINK_OPTIONS) $(LINK_LIBRARIES) $^ -o $@

$(TMP_DIR)/mc.xs.o: $(TMP_DIR)/mc.xs.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -u / -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.o: $(TMP_DIR)/mc.resources.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(C_INCLUDES) $< -o $@

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -r mc.resources.c

$(LIB_DIR)/%.cpp.o: %.cpp
	@echo "# cpp" $(<F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.%.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
