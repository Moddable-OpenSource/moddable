#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

HOST_OS := $(shell uname)

TOOLS_ROOT ?= $(BASE)/toolchains/gnu_arm/4.9_2015q3

INC_DIRS = \
	$(SDK_BASE)/platform/emlib/inc \
	$(SDK_BASE)/platform/CMSIS/Include \
	$(SDK_BASE)/hardware/kit/common/bsp \
	$(SDK_BASE)/hardware/kit/common/drivers \
	$(HWINC) \
	$(HWKIT) \
	$(XS_DIR)/sources/pcre \
	$(XS_DIR)/../modules/base/instrumentation \
	$(BUILD_DIR)/devices/gecko

#	$(SDK_BASE)/util/third_party/freertos/Source/include \

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
	$(LIB_DIR)/xsmc.c.o
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/sources/gecko \
	$(BUILD_DIR)/devices/gecko
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsgecko.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/gecko/xsPlatform.h
HEADERS += $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)/bin
TOOLS_PREFIX = arm-none-eabi

CC  = $(TOOLS_BIN)/$(TOOLS_PREFIX)-gcc
CPP = $(TOOLS_BIN)/$(TOOLS_PREFIX)-g++
LD  = $(CPP)
AR  = $(TOOLS_BIN)/$(TOOLS_PREFIX)-ar

AR_FLAGS = crs

BUILDCLUT = $(BUILD_DIR)/bin/mac/debug/buildclut
COMPRESSBMF = $(BUILD_DIR)/bin/mac/debug/compressbmf
RLE4ENCODE = $(BUILD_DIR)/bin/mac/debug/rle4encode
MCLOCAL = $(BUILD_DIR)/bin/mac/debug/mclocal
MCREZ = $(BUILD_DIR)/bin/mac/debug/mcrez
PNG2BMP = $(BUILD_DIR)/bin/mac/debug/png2bmp
IMAGE2CS = $(BUILD_DIR)/bin/mac/debug/image2cs
XSC = $(BUILD_DIR)/bin/mac/debug/xsc
XSID = $(BUILD_DIR)/bin/mac/debug/xsid
XSL = $(BUILD_DIR)/bin/mac/debug/xsl

#	-DmxNoConsole=1
C_DEFINES = \
	-DDEBUG_EFM=1 \
	-Dgecko=1 \
	-D$(HWPART)=1 \
	-U__STRICT_ANSI__ \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
ifeq ($(DEBUG),1)
#	C_DEFINES += -DDEBUG_EFM=1 -DDEBUG=1 -DmxDebug=1
	C_DEFINES += -DDEBUG=1 -DmxDebug=1
	C_FLAGS += $(HW_DEBUG_OPT)
else
	C_FLAGS += $(HW_OPT)
endif
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif

sp :=  
sp += 
qs = $(subst ?,\$(sp),$1)
C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(INC_DIRS) $(SDK_DIRS) $(XS_DIRS) $(LIB_DIR) $(TMP_DIR),-I$(call qs,$(dir)))

C_FLAGS += -gdwarf-2 -mcpu=$(HWCPU) -mthumb -std=c99 \
    -DDEBUG=1 \
    -Wall -c -fmessage-length=0 -mno-sched-prolog \
    -fno-builtin -ffunction-sections -fdata-sections \
    -MMD -MP \
	$(DEV_C_FLAGS)

#	-mfpu=fpv4-sp-d16 \
#	-mfloat-abi=softfp \

C_FLAGS_NODATASECTION = $(C_FLAGS)

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
SRC_GIT_VERSION = $(call git_description,$(BASE)/sources)
ESP_GIT_VERSION = $(call git_description,$(ARDUINO_ROOT))
time_string = $(shell perl -e 'use POSIX qw(strftime); print strftime($(1), localtime());')
BUILD_DATE = $(call time_string,"%Y-%m-%d")
BUILD_TIME = $(call time_string,"%H:%M:%S")
MEM_USAGE = \
  'while (<>) { \
      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
	 }\
	 print "\# Memory usage\n";\
	 print sprintf("\#  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

VPATH += $(SDK_DIRS) $(XS_DIRS)

.PHONY: all	

all: $(LIB_DIR) $(BIN_DIR)/xs_gecko.a
	
$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h
	
$(BIN_DIR)/xs_gecko.a: $(SDK_OBJ) $(XS_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld xs_gecko.bin"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)/buildinfo.c -o $(LIB_DIR)/buildinfo.c.o
	$(AR) $(AR_FLAGS) $(BIN_DIR)/xs_gecko.a $^ $(LIB_DIR)/buildinfo.c.o


$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.S.o: %.S
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $< -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.cpp.o: %.cpp
	@echo "# cpp" $(<F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(LIB_DIR)/%.cpp.o: %.ino
	@echo "# cpp" $(<F)
	$(CPP) -x c++ -include $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)/$(TOOLS_PREFIX)-objcopy --rename-section .data.gxKeys=.rodata.gxKeys --rename-section .data.gxNames=.rodata.gxNames --rename-section .data.gxGlobals=.rodata.gxGlobals $@.unmapped $@
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp32 -r mc.resources.c
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

