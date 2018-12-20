#
# Copyright (c) 2016-2018  Moddable Tech, Inc.
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

UPLOAD_SPEED ?= 921600
DEBUGGER_SPEED ?= 460800

ESP32_BASE ?= $(HOME)/esp32
IDF_PATH ?= $(ESP32_BASE)/esp-idf
export IDF_PATH
TOOLS_ROOT ?= $(ESP32_BASE)/xtensa-esp32-elf
PLATFORM_DIR = $(MODDABLE)/build/devices/esp32

ifeq ($(DEBUG),1)
	IDF_BUILD_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/debug/idf
	PROJ_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/debug/xsProj
else
	IDF_BUILD_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/release/idf
	PROJ_DIR = $(BUILD_DIR)/tmp/$(FULLPLATFORM)/release/xsProj
endif

ifeq ($(MAKEFLAGS_JOBS),)
	MAKEFLAGS_JOBS = --jobs
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

INC_DIRS = \
 	$(IDF_PATH)/components \
	$(IDF_PATH)/components/heap/include \
 	$(IDF_PATH)/components/driver/include \
 	$(IDF_PATH)/components/soc/esp32/include \
 	$(IDF_PATH)/components/soc/esp32/include/soc \
 	$(IDF_PATH)/components/soc/include \
 	$(IDF_PATH)/components/esp32/include \
 	$(IDF_PATH)/components/freertos \
 	$(IDF_PATH)/components/freertos/include \
 	$(IDF_PATH)/components/freertos/include/freertos \
 	$(IDF_PATH)/components/lwip/lwip/src/include \
 	$(IDF_PATH)/components/lwip/port/esp32/include \
 	$(IDF_PATH)/components/lwip/include/apps \
 	$(IDF_PATH)/components/lwip/include/lwip \
 	$(IDF_PATH)/components/lwip/include/lwip/port \
 	$(IDF_PATH)/components/mbedtls/include \
 	$(IDF_PATH)/components/spi_flash/include \
 	$(IDF_PATH)/components/vfs/include \
 	$(IDF_PATH)/components/tcpip_adapter/include \
 	$(IDF_PATH)/components/tcpip_adapter \
 	$(IDF_PATH)/components/bt/include \
 	$(IDF_PATH)/components/bt/bluedroid/api/include \
 	$(IDF_PATH)/components/bt/bluedroid/api/include/api \
	$(IDF_PATH)/components/newlib/include \
	$(IDF_PATH)/components/newlib/platform_include
    
XS_OBJ = \
	$(LIB_DIR)/xsHost.c.o \
	$(LIB_DIR)/xsPlatform.c.o \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArguments.c.o \
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
	$(LIB_DIR)/xsre.c.o \
	$(LIB_DIR)/xsmc.c.o \
	$(LIB_DIR)/e_pow.c.o
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/platforms/esp \
	$(IDF_BUILD_DIR)/include \
	$(PLATFORM_DIR)/lib/pow
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsesp.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/esp/xsPlatform.h
HEADERS += $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)/bin
CC  = $(TOOLS_BIN)/xtensa-esp32-elf-gcc
CPP = $(TOOLS_BIN)/xtensa-esp32-elf-g++
LD  = $(CPP)
AR  = $(TOOLS_BIN)/xtensa-esp32-elf-ar
OBJCOPY = $(TOOLS_BIN)/xtensa-esp32-elf-objcopy
ESPTOOL = $(IDF_PATH)/components/esptool_py/esptool/esptool.py

AR_FLAGS = crs

ifeq ($(HOST_OS),Darwin)
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/release
else
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin/release
endif
BUILDCLUT = $(MODDABLE_TOOLS_DIR)/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)/wav2maud
BLES2GATT = $(MODDABLE_TOOLS_DIR)/bles2gatt
XSC = $(MODDABLE_TOOLS_DIR)/xsc
XSID = $(MODDABLE_TOOLS_DIR)/xsid
XSL = $(MODDABLE_TOOLS_DIR)/xsl

#	-DICACHE_FLASH
#	-DmxNoConsole=1
C_DEFINES = \
	-D__ets__ \
	-U__STRICT_ANSI__ \
	-DESP32=1 \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
ifeq ($(DEBUG),1)
	C_DEFINES += -DmxDebug=1
endif
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif
C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(INC_DIRS) $(SDK_DIRS) $(XS_DIRS) $(LIB_DIR) $(TMP_DIR),-I$(dir))

C_COMMON_FLAGS ?= -c -Os -g \
	-Wno-unused-variable \
	-Wpointer-arith \
	-Wl,-EL \
	-fno-inline-functions \
	-nostdlib \
	-mlongcalls \
	-mtext-section-literals \
	-falign-functions=4 \
	-MMD \
	-fdata-sections \
	-ffunction-sections \
	-fno-jump-tables \
	-fstrict-volatile-bitfields \
	-DWITH_POSIX \
	-DHAVE_CONFIG_H \
	-D BOOTLOADER_BUILD=1 \
	-DESP_PLATFORM \
	-MP

C_FLAGS ?= $(C_COMMON_FLAGS) \
	-Wno-implicit-function-declaration \
	-std=gnu99

CPP_FLAGS ?= $(C_COMMON_FLAGS)

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

PARTITIONS_FILE ?= $(PROJ_DIR_TEMPLATE)/partitions.csv

PROJ_DIR_TEMPLATE = $(BUILD_DIR)/devices/esp32/xsProj
PROJ_DIR_FILES = \
	$(PROJ_DIR)/main/main.c \
	$(PROJ_DIR)/main/component.mk \
	$(PROJ_DIR)/partitions.csv \
	$(PROJ_DIR)/Makefile

ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)
		DO_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		DO_LAUNCH = bash -c "serial2xsbug `/usr/bin/grep ^CONFIG_ESPTOOLPY_PORT $(PROJ_DIR)/sdkconfig | /usr/bin/grep -o '"[^"]*"' | tr -d '"'` $(DEBUGGER_SPEED) 8N1 $(IDF_BUILD_DIR)/xs_esp32.elf $(TOOLS_ROOT)/bin/xtensa-esp32-elf-gdb"
	else
		KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)
		DO_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
		DO_LAUNCH = bash -c "serial2xsbug `grep ^CONFIG_ESPTOOLPY_PORT $(PROJ_DIR)/sdkconfig | grep -o '"[^"]*"' | tr -d '"'` $(DEBUGGER_SPEED) 8N1"
	endif
else
	KILL_SERIAL_2_XSBUG = 
	DO_XSBUG = 
	DO_LAUNCH = cd $(PROJ_DIR); IDF_BUILD_DIR=$(IDF_BUILD_DIR) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make monitor
endif

SDKCONFIGPATH ?= $(PROJ_DIR)
SDKCONFIG = $(SDKCONFIGPATH)/sdkconfig.defaults
SDKCONFIGPRIOR = $(SDKCONFIGPATH)/sdkconfig.defaults.prior
SDKCONFIG_H=$(IDF_BUILD_DIR)/include/sdkconfig.h


.NOTPARALLEL: $(SDKCONFIG_H)

all: projDir $(BLE) $(SDKCONFIG_H) $(LIB_DIR) $(BIN_DIR)/xs_esp32.a
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	-@rm $(IDF_BUILD_DIR)/xs_esp32.elf 2>/dev/null
	-@rm $(BIN_DIR)/xs_esp32.elf 2>/dev/null
	-@mkdir -p $(IDF_BUILD_DIR) 2>/dev/null
	cp $(BIN_DIR)/xs_esp32.a $(IDF_BUILD_DIR)/.
	touch $(PROJ_DIR)/main/main.c
	cd $(PROJ_DIR) ; IDF_BUILD_DIR=$(IDF_BUILD_DIR) DEBUG=$(DEBUG) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make flash;
	-cp $(IDF_BUILD_DIR)/xs_esp32.map $(BIN_DIR)
	-cp $(IDF_BUILD_DIR)/xs_esp32.bin $(BIN_DIR)
	-cp $(IDF_BUILD_DIR)/partitions.bin $(BIN_DIR)
	$(DO_LAUNCH)

$(SDKCONFIG_H): $(SDKCONFIG_FILE)
	if ! test -s $(SDKCONFIGPRIOR) ; then cp $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR); fi
	if ! test -s $(IDF_BUILD_DIR)/; then rm -f $(SDKCONFIGPRIOR); fi
	if ! test -s $(SDKCONFIG_H) \
		|| ! cmp -s "$(SDKCONFIGPRIOR)" "$(SDKCONFIG_FILE)" \
		|| ! cmp -s "$(PROJ_DIR)/sdkconfig" "$(SDKCONFIGPATH)/sdkconfig.old"; then \
		rm $(PROJ_DIR)/sdkconfig; \
		cp $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR); \
		echo "# Running GENCONFIG..." ; cd $(PROJ_DIR) ; IDF_BUILD_DIR=$(IDF_BUILD_DIR) BATCH_BUILD=1 DEBUG=$(DEBUG) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE) make defconfig; \
		mv $(PROJ_DIR)/sdkconfig.old $(SDKCONFIGPATH)/sdkconfig.old;  \
	fi

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h
	
$(BIN_DIR)/xs_esp32.a: $(SDK_OBJ) $(XS_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld xs_esp.bin"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)/buildinfo.c -o $(LIB_DIR)/buildinfo.c.o
	$(AR) $(AR_FLAGS) $(BIN_DIR)/xs_esp32.a $^ $(LIB_DIR)/buildinfo.c.o

projDir: $(PROJ_DIR) $(PROJ_DIR_FILES) $(PARTITIONS_FILE)

$(PROJ_DIR): $(PROJ_DIR_TEMPLATE)
	cp -r $(PROJ_DIR_TEMPLATE) $(PROJ_DIR)
	cp $(PARTITIONS_FILE) $(PROJ_DIR)/partitions.csv

$(PROJ_DIR)/partitions.csv: $(PARTITIONS_FILE)
	cp $(PARTITIONS_FILE) $(PROJ_DIR)/partitions.csv

$(PROJ_DIR)/main/main.c: $(PROJ_DIR_TEMPLATE)/main/main.c
	cp -f $? $@

$(PROJ_DIR)/main/component.mk: $(PROJ_DIR_TEMPLATE)/main/component.mk
	cp -f $? $@

$(PROJ_DIR)/Makefile: $(PROJ_DIR_TEMPLATE)/Makefile
	cp -f $? $@

$(XS_OBJ): $(SDKCONFIG_H) $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp32 -r mc.resources.c
	
MAKEFLAGS += $(MAKEFLAGS_JOBS)
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

