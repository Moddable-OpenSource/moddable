#
# Copyright (c) 2016-2023  Moddable Tech, Inc.
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

XSBUG_HOST ?= localhost
XSBUG_PORT ?= 5002

USE_USB ?= 0
USB_VENDOR_ID ?= beef
USB_PRODUCT_ID ?= 1cee
PROGRAMMING_VID ?= 303a
PROGRAMMING_PID ?= 1001

EXPECTED_ESP_IDF ?= v4.4.3

# ESP32_SUBCLASS is to find some include files in IDFv4
# values include esp32, esp32s3 and esp32s2
ESP32_SUBCLASS ?= esp32
# $(warning ESP32_SUBCLASS $(ESP32_SUBCLASS))

ifeq ("$(ESP32_SUBCLASS)","esp32c3")
	ESP_ARCH = riscv
	GXX_PREFIX = riscv32-esp
else
	ESP_ARCH = xtensa
	GXX_PREFIX = xtensa-$(ESP32_SUBCLASS)
endif

ifeq ($(VERBOSE),1)
	CMAKE_LOG_LEVEL = VERBOSE
	IDF_PY_LOG_FLAG = -v
else
	CMAKE_LOG_LEVEL = ERROR
	IDF_PY_LOG_FLAG = -n
endif

PLATFORM_DIR = $(MODDABLE)/build/devices/esp32

ifeq ($(IDF_PATH),)
$(error $$IDF_PATH not set. See set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/esp32.md)
endif

IDF_VERSION := $(shell bash -c "cd $(IDF_PATH) && git describe --always --abbrev=0")

ifeq ($(IDF_VERSION),) 
$(error Could not detect ESP-IDF version at $$IDF_PATH: $(IDF_PATH).)
endif

unexport LDFLAGS
unexport LD_LIBRARY_PATH
unexport CPPFLAGS

PROJ_DIR = $(TMP_DIR)/xsProj-$(ESP32_SUBCLASS)

BLD_DIR = $(PROJ_DIR)/build

ifeq ($(MAKEFLAGS_JOBS),)
	MAKEFLAGS_JOBS = --jobs
endif

USB_OPTION=

SDKCONFIG_H_DIR = $(BLD_DIR)/config
ifeq ("$(ESP32_SUBCLASS)","esp32c3")
	ESP32_TARGET = 4
else
ifeq ("$(ESP32_SUBCLASS)","esp32s3")
	ESP32_TARGET = 3
	ifeq ($(USE_USB),1) 
		USB_OPTION = -DUSE_USB=1
	endif
else
	ifeq ("$(ESP32_SUBCLASS)","esp32s2")
		ESP32_TARGET = 2
	else
		ESP32_TARGET = 1
	endif
endif
endif


INC_DIRS = \
 	$(IDF_PATH)/components \
 	$(IDF_PATH)/components/bootloader_support/include \
 	$(IDF_PATH)/components/bt/include \
	$(IDF_PATH)/components/bt/include/$(ESP32_SUBCLASS)/include \
 	$(IDF_PATH)/components/bt/host/bluedroid/api/include \
 	$(IDF_PATH)/components/bt/host/bluedroid/api/include/api \
 	$(IDF_PATH)/components/driver/include \
 	$(IDF_PATH)/components/driver/include/driver \
	$(IDF_PATH)/components/driver/$(ESP32_SUBCLASS)/include \
 	$(IDF_PATH)/components/driver/$(ESP32_SUBCLASS)/include/driver \
 	$(IDF_PATH)/components/esp_common/include \
 	$(IDF_PATH)/components/$(ESP32_SUBCLASS) \
 	$(IDF_PATH)/components/$(ESP32_SUBCLASS)/include \
 	$(IDF_PATH)/components/esp_event/include \
 	$(IDF_PATH)/components/esp_eth/include \
 	$(IDF_PATH)/components/esp_hw_support/include \
 	$(IDF_PATH)/components/esp_hw_support/include/soc \
	$(IDF_PATH)/components/esp_lcd/include \
 	$(IDF_PATH)/components/esp_netif/include \
 	$(IDF_PATH)/components/esp_pm/include \
 	$(IDF_PATH)/components/esp_ringbuf/include \
 	$(IDF_PATH)/components/esp_rom/include \
 	$(IDF_PATH)/components/esp_rom/include/$(ESP32_SUBCLASS) \
 	$(IDF_PATH)/components/esp_system/include \
 	$(IDF_PATH)/components/esp_timer/include \
 	$(IDF_PATH)/components/esp_wifi/include \
 	$(IDF_PATH)/components/$(ESP_ARCH)/include \
	$(IDF_PATH)/components/$(ESP_ARCH)/$(ESP32_SUBCLASS)/include \
 	$(IDF_PATH)/components/freertos \
 	$(IDF_PATH)/components/freertos/include \
 	$(IDF_PATH)/components/freertos/include/freertos \
 	$(IDF_PATH)/components/freertos/port \
 	$(IDF_PATH)/components/freertos/port/$(ESP_ARCH)/include \
	$(IDF_PATH)/components/freertos/include/esp_additions \
	$(IDF_PATH)/components/freertos/include/esp_additions/freertos \
	$(IDF_PATH)/components/freertos/port/$(ESP_ARCH)/include/freertos \
	$(IDF_PATH)/components/hal/include \
	$(IDF_PATH)/components/hal/$(ESP32_SUBCLASS)/include \
	$(IDF_PATH)/components/hal/platform_port/include \
	$(IDF_PATH)/components/heap/include \
 	$(IDF_PATH)/components/log/include \
 	$(IDF_PATH)/components/lwip/include/apps/ \
	$(IDF_PATH)/components/lwip/include/apps/sntp \
 	$(IDF_PATH)/components/lwip/lwip/src/include/ \
 	$(IDF_PATH)/components/lwip/port/esp32/ \
 	$(IDF_PATH)/components/lwip/port/esp32/include/ \
 	$(IDF_PATH)/components/mbedtls/mbedtls/include/ \
 	$(IDF_PATH)/components/newlib/include \
 	$(IDF_PATH)/components/newlib/platform_include \
 	$(IDF_PATH)/components/bt/host/nimble/esp-hci/include \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/nimble/host/include \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/nimble/host/services/gap/include \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/nimble/host/src \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/nimble/include \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/nimble/include/nimble \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/porting/nimble/include \
 	$(IDF_PATH)/components/bt/host/nimble/nimble/porting/npl/freertos/include \
 	$(IDF_PATH)/components/bt/host/nimble/port/include \
        $(IDF_PATH)/components/soc/$(ESP32_SUBCLASS) \
 	$(IDF_PATH)/components/soc/$(ESP32_SUBCLASS)/include \
	$(IDF_PATH)/components/soc/$(ESP32_SUBCLASS)/include/soc \
 	$(IDF_PATH)/components/soc/include \
 	$(IDF_PATH)/components/soc/include/soc \
 	$(IDF_PATH)/components/spiffs/include \
	$(IDF_PATH)/components/fatfs/src \
	$(IDF_PATH)/components/fatfs/vfs \
	$(IDF_PATH)/components/wear_levelling/include \
	$(IDF_PATH)/components/sdmmc/include \
 	$(IDF_PATH)/components/spi_flash/include \
 	$(IDF_PATH)/components/tcpip_adapter/include \
 	$(IDF_PATH)/components/tcpip_adapter \
 	$(IDF_PATH)/components/tinyusb/additions/include
	
XS_OBJ = \
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
	$(LIB_DIR)/xsmc.c.o \
	$(LIB_DIR)/e_pow.c.o
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/platforms/esp \
	$(XS_DIR)/platforms/mc \
	$(SDKCONFIG_H_DIR) \
	$(PLATFORM_DIR)/lib/pow
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/esp/xsHost.h \
	$(XS_DIR)/platforms/mc/xsHosts.h \
	$(XS_DIR)/platforms/esp/xsPlatform.h
HEADERS += $(XS_HEADERS)

CC = $(GXX_PREFIX)-elf-gcc
CPP = $(GXX_PREFIX)-elf-g++
LD = $(CPP)
AR = $(GXX_PREFIX)-elf-ar
OBJCOPY = $(GXX_PREFIX)-elf-objcopy
OBJDUMP = $(GXX_PREFIX)-elf-objdump

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
	-DESP32=$(ESP32_TARGET) \
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

ifeq ("$(ESP_ARCH)","riscv")
C_COMMON_FLAGS +=	\
	-march=rv32imc
else
C_COMMON_FLAGS +=	\
 	-mlongcalls \
	-mtext-section-literals
endif

C_FLAGS ?= $(C_COMMON_FLAGS) \
	-Wno-implicit-function-declaration \
	-std=gnu99 \
	$(C_FLAGS_SUBPLATFORM)

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

.PHONY: all bootloaderCheck

PROJ_DIR_TEMPLATE = $(BUILD_DIR)/devices/esp32/xsProj-$(ESP32_SUBCLASS)
PROJ_DIR_FILES = \
	$(PROJ_DIR)/main/main.c \
	$(PROJ_DIR)/main/component.mk \
	$(PROJ_DIR)/main/CMakeLists.txt \
	$(PROJ_DIR)/CMakeLists.txt \
	$(PROJ_DIR)/partitions.csv \
	$(PROJ_DIR)/Makefile

ifneq ($(BOOTLOADERPATH),)
	PROJ_DIR_FILES += $(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c
endif

ifeq ($(UPLOAD_PORT),)
	PORT_SET =
	SERIAL2XSBUG_PORT = $$PORT_USED
else
	PORT_SET = -p $(UPLOAD_PORT)
	SERIAL2XSBUG_PORT = $(UPLOAD_PORT)
endif

BUILD_AND_FLASH_CMD = idf.py $(PORT_SET) -b $(UPLOAD_SPEED) $(IDF_PY_LOG_FLAG) build flash -D mxDebug=$(DEBUG) -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR=$(TMP_DIR) -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) $(USB_OPTION)
BUILD_CMD = idf.py $(IDF_PY_LOG_FLAG) build -D mxDebug=$(DEBUG) -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR=$(TMP_DIR) -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) -D SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE) -D SDKCONFIG_HEADER="$(SDKCONFIG_H)"
BUILD_ERR = "ESP-IDF Build Failed"
DEPLOY_CMD = idf.py $(PORT_SET) -b $(UPLOAD_SPEED) $(IDF_PY_LOG_FLAG) flash -D mxDebug=$(DEBUG) -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR=$(TMP_DIR) -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED)
IDF_RECONFIGURE_CMD = idf.py $(IDF_PY_LOG_FLAG) reconfigure -D SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE) -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D IDF_TARGET=$(ESP32_SUBCLASS) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) $(USB_OPTION)
RELEASE_LAUNCH_CMD = idf.py $(PORT_SET) $(IDF_PY_LOG_FLAG) monitor
PARTITIONS_BIN = partition-table.bin
PARTITIONS_PATH = $(BLD_DIR)/partition_table/$(PARTITIONS_BIN)

ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		DO_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		ifeq ($(USE_USB),1)
			DO_LAUNCH = bash -c "serial2xsbug $(USB_VENDOR_ID):$(USB_PRODUCT_ID) $(DEBUGGER_SPEED) 8N1 -elf $(PROJ_DIR)/build/xs_esp32.elf -bin $(GXX_PREFIX)-elf-gdb"
			LOG_LAUNCH = bash -c \"serial2xsbug $(USB_VENDOR_ID):$(USB_PRODUCT_ID) $(DEBUGGER_SPEED) 8N1 -elf $(PROJ_DIR)/build/xs_esp32.elf -bin $(GXX_PREFIX)-elf-gdb\"
		else
			DO_LAUNCH = bash -c "XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(SERIAL2XSBUG_PORT) $(DEBUGGER_SPEED) 8N1 -elf $(PROJ_DIR)/build/xs_esp32.elf -bin $(GXX_PREFIX)-elf-gdb"
			LOG_LAUNCH = bash -c \"XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(SERIAL2XSBUG_PORT) $(DEBUGGER_SPEED) 8N1 -elf $(PROJ_DIR)/build/xs_esp32.elf -bin $(GXX_PREFIX)-elf-gdb\"
		endif

		ifeq ($(XSBUG_LOG),1)
			DO_LAUNCH := cd $(MODDABLE)/tools/xsbug-log && node xsbug-log $(LOG_LAUNCH)
		endif

	### Linux
	else
		DO_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
		ifeq ($(USE_USB),1)
#			DO_LAUNCH = bash -c "serial2xsbug $(USB_VENDOR_ID):$(USB_PRODUCT_ID) $(DEBUGGER_SPEED) 8N1"
			DO_LAUNCH = bash -c "PATH=\"$(PLATFORM_DIR)/config:$(PATH)\"; connectToXsbugLinux $(USB_VENDOR_ID) $(USB_PRODUCT_ID) $(XSBUG_LOG)"
			PROGRAMMING_MODE = bash -c "PATH=\"$(PLATFORM_DIR)/config:$(PATH)\"; programmingModeLinux $(PROGRAMMING_VID) $(PROGRAMMING_PID) $(XSBUG_LOG)"
		else
			LOG_LAUNCH = bash -c \"XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(SERIAL2XSBUG_PORT) $(DEBUGGER_SPEED) 8N1\"

			ifeq ($(XSBUG_LOG),1)
				DO_LAUNCH := cd $(MODDABLE)/tools/xsbug-log && node xsbug-log $(LOG_LAUNCH)
			else
				DO_LAUNCH = bash -c "XSBUG_PORT=$(XSBUG_PORT) XSBUG_HOST=$(XSBUG_HOST) serial2xsbug $(SERIAL2XSBUG_PORT) $(DEBUGGER_SPEED) 8N1"
			endif

		endif
	endif

	ifeq ($(XSBUG_LOG),1)
		DO_XSBUG = 
	endif

else
	DO_XSBUG = 
	DO_LAUNCH = cd $(PROJ_DIR); $(RELEASE_LAUNCH_CMD)
endif
KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)

SDKCONFIGPATH ?= $(PROJ_DIR)
SDKCONFIG = $(SDKCONFIGPATH)/sdkconfig.defaults
SDKCONFIG_H = $(SDKCONFIG_H_DIR)/sdkconfig.h

all: precursor
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	cd $(PROJ_DIR) ; $(BUILD_CMD) || (echo $(BUILD_ERR) && exit 1)
	$(OBJDUMP) -t $(BLD_DIR)/xs_esp32.elf > $(BIN_DIR)/xs_$(ESP32_SUBCLASS).sym 2> /dev/null
	-cp $(BLD_DIR)/xs_esp32.map $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/xs_esp32.bin $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/xs_esp32.elf $(BIN_DIR) 2> /dev/null
	-cp $(PARTITIONS_PATH) $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/bootloader/bootloader.bin $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/ota_data_initial.bin $(BIN_DIR) 2>/dev/null
	$(PROGRAMMING_MODE)
	cd $(PROJ_DIR) ; bash -c "set -o pipefail; $(DEPLOY_CMD) | tee $(PROJ_DIR)/flashOutput"
	PORT_USED=$$(grep 'Serial port' $(PROJ_DIR)/flashOutput | awk 'END{print($$3)}'); \
	cd $(PROJ_DIR); \
	$(DO_LAUNCH)

deploy: 
	if ! test -e $(BIN_DIR)/xs_esp32.bin ; then (echo "Please build before deploy" && exit 1) fi
	@echo "# uploading to $(ESP32_SUBCLASS)"
	$(KILL_SERIAL_2_XSBUG)
	-cd $(PROJ_DIR) ; $(DEPLOY_CMD) | tee $(PROJ_DIR)/flashOutput

xsbug:
	@echo "# starting xsbug"
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	PORT_USED=$$(grep 'Serial port' $(PROJ_DIR)/flashOutput | awk 'END{print($$3)}'); \
	$(DO_LAUNCH)

prepareOutput:
	-@rm $(PROJ_DIR)/xs_esp32.elf 2>/dev/null
	-@rm $(BIN_DIR)/xs_esp32.elf 2>/dev/null
#	-@mkdir -p $(PROJ_DIR) 2>/dev/null

DUMP_VARS:
	echo "#\n#\n# vars\n#\n#\n"
	echo "# SDKCONFIGPATH=$(SDKCONFIGPATH)"
	echo "# CONFIGDIR=$(CONFIGDIR)"
	echo "# SDKCONFIG_FILE=$(SDKCONFIG_FILE)"
	echo "# PROJ_DIR_FILES is $(PROJ_DIR_FILES)"
	echo "# IDF_RECONFIGURE_CMD is $(IDF_RECONFIGURE_CMD)"
	echo "# SDKCONFIG_H_DIR is $(SDKCONFIG_H_DIR)"

precursor: idfVersionCheck prepareOutput $(PROJ_DIR_FILES) bootloaderCheck $(BLE) $(SDKCONFIG_H) $(LIB_DIR) $(BIN_DIR)/xs_$(ESP32_SUBCLASS).a
	cp $(BIN_DIR)/xs_$(ESP32_SUBCLASS).a $(BLD_DIR)/.
	touch $(PROJ_DIR)/main/main.c

build: precursor
	-cd $(PROJ_DIR) ; $(BUILD_CMD)
	$(OBJDUMP) -t $(BLD_DIR)/xs_esp32.elf > $(BIN_DIR)/xs_$(ESP32_SUBCLASS).sym 2> /dev/null
	-cp $(BLD_DIR)/xs_esp32.map $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/xs_esp32.bin $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/bootloader/bootloader.bin $(BIN_DIR) 2> /dev/null
	-cp $(PARTITIONS_PATH) $(BIN_DIR) 2> /dev/null
	-cp $(BLD_DIR)/ota_data_initial.bin $(BIN_DIR) 2> /dev/null
	echo "#"
	echo "# Built files at $(BIN_DIR)"
	echo "#"

clean:
	echo "# Clean project"
	-rm -rf $(BIN_DIR) 2>/dev/null
	-rm -rf $(TMP_DIR) 2>/dev/null
	-rm -rf $(LIB_DIR) 2>/dev/null	

$(SDKCONFIG_H): $(SDKCONFIG_FILE) $(PROJ_DIR_FILES)
	-rm $(PROJ_DIR)/sdkconfig 2>/dev/null
	echo "# Reconfiguring ESP-IDF..." ; cd $(PROJ_DIR) ; $(IDF_RECONFIGURE_CMD)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
$(BIN_DIR)/xs_$(ESP32_SUBCLASS).a: $(SDK_OBJ) $(XS_OBJ) $(TMP_DIR)/xsPlatform.c.o $(TMP_DIR)/xsHost.c.o $(TMP_DIR)/xsHosts.c.o $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld xs_esp32.bin"
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(TMP_DIR)/buildinfo.h
	echo '#include "buildinfo.h"' > $(TMP_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(TMP_DIR)/buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(TMP_DIR)/buildinfo.c -o $(TMP_DIR)/buildinfo.c.o
	$(AR) $(AR_FLAGS) $(BIN_DIR)/xs_$(ESP32_SUBCLASS).a $^ $(TMP_DIR)/buildinfo.c.o

bootloaderCheck:
ifneq ($(BOOTLOADERPATH),)
	if test -e $(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c ; then \
		if ! cmp -s $(BOOTLOADERPATH)/subproject/main/bootloader_start.c $(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c ; then \
			rm -rf $(PROJ_DIR)/bootloader; \
		fi ; \
	else \
		rm -rf $(PROJ_DIR)/bootloader; \
	fi
else
	if test -e $(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c ; then \
		rm -rf $(PROJ_DIR)/components; \
		rm -rf $(PROJ_DIR)/bootloader; \
	fi
endif

idfVersionCheck:
	python $(PROJ_DIR_TEMPLATE)/versionCheck.py $(EXPECTED_ESP_IDF) $(IDF_VERSION) || (echo "Expected ESP-IDF $(EXPECTED_ESP_IDF), found $(IDF_VERSION)"; exit 1)


$(PROJ_DIR): $(PROJ_DIR_TEMPLATE)
	cp -r $(PROJ_DIR_TEMPLATE)/* $(PROJ_DIR)/

$(PROJ_DIR)/main:
	mkdir -p $(PROJ_DIR)/main

$(PROJ_DIR)/main/main.c: $(PROJ_DIR)/main $(PROJ_DIR_TEMPLATE)/main/main.c
	cp -f $(PROJ_DIR_TEMPLATE)/main/main.c $@

$(PROJ_DIR)/main/component.mk: $(PROJ_DIR)/main $(PROJ_DIR_TEMPLATE)/main/component.mk
	cp -f $(PROJ_DIR_TEMPLATE)/main/component.mk $@

$(PROJ_DIR)/main/CMakeLists.txt: $(PROJ_DIR)/main $(PROJ_DIR_TEMPLATE)/main/CMakeLists.txt
	cp -f $(PROJ_DIR_TEMPLATE)/main/CMakeLists.txt $@

$(PROJ_DIR)/CMakeLists.txt: $(PROJ_DIR_TEMPLATE)/CMakeLists.txt
	cp -f $(PROJ_DIR_TEMPLATE)/CMakeLists.txt $@

$(PROJ_DIR)/Makefile: $(PROJ_DIR_TEMPLATE)/Makefile
	cp -f $? $@

$(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c: $(PROJ_DIR) bootloaderCheck $(BOOTLOADERPATH)/subproject/main/bootloader_start.c
	echo Using custom bootloader: $(BOOTLOADERPATH)
	mkdir -p $(PROJ_DIR)/components/bootloader
	cp -fr $(BOOTLOADERPATH)/* $(PROJ_DIR)/components/bootloader/
	touch $(PROJ_DIR)/components/bootloader/subproject/main/bootloader_start.c

$(XS_OBJ): $(SDKCONFIG_H) $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/xsPlatform.c.o: xsPlatform.c $(XS_HEADERS) $(TMP_DIR)/mc.defines.h $(TMP_DIR)/mc.format.h $(TMP_DIR)/mc.rotation.h $(SDKCONFIG_H) 
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/xsHost.c.o: xsHost.c $(XS_HEADERS) $(TMP_DIR)/mc.defines.h $(TMP_DIR)/mc.format.h $(TMP_DIR)/mc.rotation.h $(SDKCONFIG_H) 
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/xsHosts.c.o: xsHosts.c $(XS_HEADERS) $(TMP_DIR)/mc.defines.h $(TMP_DIR)/mc.format.h $(TMP_DIR)/mc.rotation.h $(SDKCONFIG_H) 
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.c.o: %.c $(SDKCONFIG_H) 
	@echo "# cc" $(<F) "(strings in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c $(SDKCONFIG_H) 
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST) $(SDKCONFIG_H) 
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST) $(SDKCONFIG_H) 
	@echo "# mcrez resources"
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -p $(ESP32_SUBCLASS) -r mc.resources.c
	
MAKEFLAGS += $(MAKEFLAGS_JOBS)
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

