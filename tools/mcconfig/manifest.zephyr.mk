#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

ifndef ZEPHYR_BASE
$(error Missing Zephyr base, did you source zephyr-env.sh? )
endif

# Default target is FRDM-K64F
BOARD ?= frdm_k64f
export BOARD

ZEPHYR_APP_DIR = $(BUILD_DIR)/devices/zephyr
#ZEPHYR_OUTPUT_DIR = $(BUILD_DIR)/tmp/zephyr/outdir
ZEPHYR_OUTPUT_DIR = $(TMP_DIR)/zephyr

ZEPHYR_MAKE = $(MAKE) --print-directory O="$(ZEPHYR_OUTPUT_DIR)" XS_LIB_DIR="$(TMP_DIR)" -C $(ZEPHYR_APP_DIR) -f $(ZEPHYR_APP_DIR)/Makefile

# Zephyr build environment
include $(ZEPHYR_OUTPUT_DIR)/Makefile.export

LIB_DIR = $(BUILD_DIR)/tmp/zephyr/debug/lib

XS_OBJ = \
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
	$(LIB_DIR)/xspcre.c.o \
	$(LIB_DIR)/xsmc.c.o
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/sources/pcre \
	$(XS_DIR)/sources/zephyr
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/zephyr/xsPlatform.h
HEADERS += $(XS_HEADERS)

INC_DIRS += \
	$(ZEPHYR_APP_DIR)/src

# XS6 tools
BUILDCLUT = buildclut
COMPRESSBMF = compressbmf
RLE4ENCODE = rle4encode
MCREZ = mcrez
PNG2BMP = png2bmp
IMAGE2CS = image2cs
XSC = xsc
XSID = xsid
XSL = xsl

C_DEFINES = \
	-D__ZEPHYR__=1 \
	-U__STRICT_ANSI__ \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
ifeq ($(DEBUG),1)
#	C_DEFINES += -DmxDebug=1
endif
ifeq ($(INSTRUMENT),1)
#	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif

C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(INC_DIRS) $(SDK_DIRS) $(XS_DIRS) $(LIB_DIR) $(TMP_DIR),-I$(dir))

C_FLAGS_NODATASECTION = $(subst -fdata-sections,,$(subst -ffunction-sections,,$(KBUILD_CFLAGS)))

C_FLAGS_NODATASECTION += $(NOSTDINC_FLAGS) $(subst -I,-isystem,$(ZEPHYRINCLUDE))
C_FLAGS = $(C_FLAGS_NODATASECTION) -ffunction-sections -fdata-sections
S_FLAGS = $(KBUILD_AFLAGS)

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
SRC_GIT_VERSION = $(call git_description,$(ZEPHYR_APP_DIR))
ZEPHYR_GIT_VERSION = $(call git_description,$(ZEPHYR_BASE))
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

all: zephyr
	# Copy binaries
	cp $(ZEPHYR_OUTPUT_DIR)/zephyr.elf $(BIN_DIR)
	cp $(ZEPHYR_OUTPUT_DIR)/zephyr.bin $(BIN_DIR)
	@echo "zephyr binaries are ready in $(BIN_DIR)"

zephyr: $(LIB_DIR) $(TMP_DIR)/libxs.a
	@echo "Building Zephyr kernel..."
	$(ZEPHYR_MAKE)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h

$(TMP_DIR)/libxs.a: $(XS_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS)
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.cpp
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ZEPHYR_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.cpp
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(LIB_DIR)/buildinfo.cpp -o $(LIB_DIR)/buildinfo.cpp.o
	$(AR) rcs $(TMP_DIR)/libxs.a $^
	@echo "# Versions"
	@echo "#  Zephyr:   $(ZEPHYR_GIT_VERSION)"
	@echo "#  XS:    $(SRC_GIT_VERSION)"
	# Force Zephyr build system to link again
	rm -f $(ZEPHYR_OUTPUT_DIR)/libzephyr.a

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

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p zephyr -r mc.resources.c

$(ZEPHYR_OUTPUT_DIR)/Makefile.export: $(ZEPHYR_OUTPUT_DIR)/include/config/auto.conf
	$(ZEPHYR_MAKE) outputexports

$(ZEPHYR_OUTPUT_DIR)/include/config/auto.conf:
	$(ZEPHYR_MAKE) initconfig
	@echo "Building Zephyr (arch only for sysgen)..."
	$(ZEPHYR_MAKE) arch

MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

