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

ESP_BASE ?= $(HOME)/esp
HOST_OS := $(shell uname)

XS_GIT_VERSION ?= $(shell git -C $(MODDABLE) describe --tags --always --dirty 2>/dev/null)
ESP_SDK_RELEASE ?= esp8266-2.3.0
ARDUINO_ROOT ?= $(ESP_BASE)/$(ESP_SDK_RELEASE)
ESPRESSIF_SDK_ROOT ?= $(ESP_BASE)/ESP8266_RTOS_SDK
ESP_TOOLS_SDK_ROOT = $(ARDUINO_ROOT)/tools/sdk
ARDUINO_ESP8266 = $(ARDUINO_ROOT)/cores/esp8266
TOOLS_ROOT ?= $(ESP_BASE)/toolchain/$(HOST_OS)
PLATFORM_DIR = $(MODDABLE)/build/devices/esp

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/esp/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/esp/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/esp/release/lib
	endif
endif

# serial port configuration
UPLOAD_SPEED ?= 921600
ifeq ($(HOST_OS),Darwin)
UPLOAD_PORT ?= /dev/cu.SLAB_USBtoUART
else
UPLOAD_PORT ?= /dev/ttyUSB0
endif
UPLOAD_RESET ?= nodemcu
ifeq ($(VERBOSE),1)
UPLOAD_VERB = -v
endif

# Board settings for ESP-12E module (the most common); change for other modules
FLASH_SIZE ?= 4M
FLASH_MODE ?= dio
FLASH_SPEED ?= 40
FLASH_LAYOUT ?= eagle.flash.4m.ld

# WiFi & Debug settings
WIFI_SSID ?= 
WIFI_PSK ?= 
DEBUG_IP ?= 

# End user-configurable values. Derived values below.
NET_CONFIG_FLAGS := 
ifneq ($(WIFI_SSID),)
NET_CONFIG_FLAGS += -DWIFI_SSID=$(WIFI_SSID)
endif
ifneq ($(WIFI_PSK),)
NET_CONFIG_FLAGS += -DWIFI_PSK=$(WIFI_PSK)
endif
ifneq ($(DEBUG_IP),)
comma := ,
NET_CONFIG_FLAGS += -DDEBUG_IP=$(subst .,$(comma),$(DEBUG_IP))
endif

CORE_DIR = $(ARDUINO_ROOT)/cores/esp8266
INC_DIRS = \
 	$(ESP_TOOLS_SDK_ROOT)/include \
 	$(ESP_TOOLS_SDK_ROOT)/lwip/include \
 	$(CORE_DIR) \
 	$(ARDUINO_ROOT)/variants/generic \
 	$(ARDUINO_ROOT)/cores/esp8266/spiffs
SDK_SRC = \
	$(ARDUINO_ESP8266)/abi.cpp \
	$(ARDUINO_ESP8266)/cont.S \
	$(ARDUINO_ESP8266)/cont_util.c \
	$(ARDUINO_ESP8266)/core_esp8266_main.cpp \
	$(ARDUINO_ESP8266)/core_esp8266_noniso.c \
	$(ARDUINO_ESP8266)/core_esp8266_phy.c \
	$(ARDUINO_ESP8266)/core_esp8266_postmortem.c \
	$(ARDUINO_ESP8266)/core_esp8266_si2c.c \
	$(ARDUINO_ESP8266)/core_esp8266_timer.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring_digital.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring_pwm.c \
	$(ARDUINO_ESP8266)/Esp.cpp \
	$(ARDUINO_ESP8266)/heap.c \
	$(ARDUINO_ESP8266)/libc_replacements.c \
	$(ARDUINO_ESP8266)/spiffs/spiffs_cache.c \
	$(ARDUINO_ESP8266)/spiffs/spiffs_check.c \
	$(ARDUINO_ESP8266)/spiffs/spiffs_gc.c \
	$(ARDUINO_ESP8266)/spiffs/spiffs_hydrogen.c \
	$(ARDUINO_ESP8266)/spiffs/spiffs_nucleus.c \
	$(ARDUINO_ESP8266)/time.c \
	$(ARDUINO_ESP8266)/uart.c \
	$(ARDUINO_ESP8266)/umm_malloc/umm_malloc.c \
	$(ARDUINO_ESP8266)/Schedule.cpp \
	$(PLATFORM_DIR)/lib/fmod/e_fmod.c \
	$(PLATFORM_DIR)/lib/rtc/rtctime.c \
	$(PLATFORM_DIR)/lib/tinyprintf/tinyprintf.c
SDK_SRC_SKIPPED = \
	$(ARDUINO_ESP8266)/base64.cpp \
	$(ARDUINO_ESP8266)/cbuf.cpp \
	$(ARDUINO_ESP8266)/core_esp8266_eboot_command.c \
	$(ARDUINO_ESP8266)/core_esp8266_i2s.c \
	$(ARDUINO_ESP8266)/core_esp8266_flash_utils.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring_analog.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring_pulse.c \
	$(ARDUINO_ESP8266)/core_esp8266_wiring_shift.c \
	$(ARDUINO_ESP8266)/debug.cpp \
	$(ARDUINO_ESP8266)/pgmspace.cpp \
	$(ARDUINO_ESP8266)/HardwareSerial.cpp \
	$(ARDUINO_ESP8266)/IPAddress.cpp \
	$(ARDUINO_ESP8266)/spiffs_api.cpp \
	$(ARDUINO_ESP8266)/Print.cpp \
	$(ARDUINO_ESP8266)/MD5Builder.cpp \
	$(ARDUINO_ESP8266)/Stream.cpp \
	$(ARDUINO_ESP8266)/Tone.cpp \
	$(ARDUINO_ESP8266)/Updater.cpp \
	$(ARDUINO_ESP8266)/WMath.cpp \
	$(ARDUINO_ESP8266)/WString.cpp \
	$(ARDUINO_ESP8266)/FS.cpp \
	$(ARDUINO_ESP8266)/libb64/cdecode.c \
	$(ARDUINO_ESP8266)/libb64/cencode.c \
	$(ARDUINO_ESP8266)/StreamString.cpp
	
SDK_OBJ = $(subst .ino,.cpp,$(patsubst %,$(LIB_DIR)/%.o,$(notdir $(SDK_SRC))))
SDK_DIRS = $(sort $(dir $(SDK_SRC)))
    
XS_OBJ = \
	$(LIB_DIR)/xsHost.c.o \
	$(LIB_DIR)/xsPlatform.c.o \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArray.c.o \
	$(LIB_DIR)/xsAtomics.c.o \
	$(LIB_DIR)/xsBoolean.c.o \
	$(LIB_DIR)/xsCommon.c.o \
	$(LIB_DIR)/xsDataView.c.o \
	$(LIB_DIR)/xsDate.c.o \
	$(LIB_DIR)/xsDebug.c.o \
	$(LIB_DIR)/xsError.c.o \
	$(LIB_DIR)/xsFunction.c.o \
	$(LIB_DIR)/xsGenerator.c.o \
	$(LIB_DIR)/xsGlobal.c.o \
	$(LIB_DIR)/xsJSON.c.o \
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
	$(LIB_DIR)/xsString.c.o \
	$(LIB_DIR)/xsSymbol.c.o \
	$(LIB_DIR)/xsType.c.o \
	$(LIB_DIR)/xsdtoa.c.o \
	$(LIB_DIR)/xsre.c.o \
	$(LIB_DIR)/xsmc.c.o \
	$(LIB_DIR)/main.cpp.o
XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/sources/pcre \
	$(XS_DIR)/platforms/esp \
	$(BUILD_DIR)/devices/esp
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsesp.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/esp/xsPlatform.h
HEADERS += $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)/xtensa-lx106-elf/bin
CC  = $(TOOLS_BIN)/xtensa-lx106-elf-gcc
CPP = $(TOOLS_BIN)/xtensa-lx106-elf-g++
LD  = $(CC)
AR  = $(TOOLS_BIN)/xtensa-lx106-elf-ar
OTA_TOOL = $(TOOLS_ROOT)/espota.py
ESPTOOL = $(ESP_BASE)/esptool/esptool

ifeq ($(HOST_OS),Darwin)
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/debug
else
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin/debug
endif
BUILDCLUT = $(MODDABLE_TOOLS_DIR)/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/image2cs
XSC = $(MODDABLE_TOOLS_DIR)/xsc
XSID = $(MODDABLE_TOOLS_DIR)/xsid
XSL = $(MODDABLE_TOOLS_DIR)/xsl

C_DEFINES = \
	-D__ets__ \
	-DICACHE_FLASH \
	-U__STRICT_ANSI__ \
	-DF_CPU=80000000L \
	-DARDUINO=10605 \
	-DARDUINO_ESP8266_ESP01 \
	-DARDUINO_ARCH_ESP8266 \
	-DESP8266 \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DmxNoConsole=1 \
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
C_FLAGS ?= -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99 -fdata-sections -ffunction-sections -fno-jump-tables
C_FLAGS_NODATASECTION = -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99
CPP_FLAGS ?= -c -Os -g -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections
S_FLAGS ?= -c -g -x assembler-with-cpp -MMD
LD_FLAGS ?= -g -w -Os -nostdlib -Wl,-Map=$(BIN_DIR)/main.txt -Wl,--cref -Wl,--no-check-sections -u call_user_start -Wl,-static -L$(ESP_TOOLS_SDK_ROOT)/lib -L$(MODDABLE)/build/devices/esp/sdk/ld -T$(FLASH_LAYOUT) -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,register_chipv6_phy
LD_STD_LIBS ?= -lm -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig -lwps -lcrypto -laxtls
# stdc++ used in later versions of esp8266 Arduino
LD_STD_CPP = lstdc++
ifneq ($(shell grep $(LD_STD_CPP) $(ARDUINO_ROOT)/platform.txt),)
	LD_STD_LIBS += -$(LD_STD_CPP)
endif

# Utility functions
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
	
ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		LAUNCH = debugmac
	else
		LAUNCH = debuglin
	endif
else
	LAUNCH = release
endif

.PHONY: all	

all: $(LAUNCH)

debuglin: $(LIB_DIR) $(BIN_DIR)/main.bin
	$(shell pkill serial2xsbug)
	$(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
	$(ESPTOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(BIN_DIR)/main.bin
	$(BUILD_DIR)/bin/lin/debug/serial2xsbug $(UPLOAD_PORT) 460800 8N1

debugmac: $(LIB_DIR) $(BIN_DIR)/main.bin
	$(shell pkill serial2xsbug)
	open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
	$(ESPTOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(BIN_DIR)/main.bin
	$(BUILD_DIR)/bin/mac/release/serial2xsbug $(UPLOAD_PORT) 460800 8N1 $(TMP_DIR)/main.elf $(TOOLS_BIN)
	
release: $(LIB_DIR) $(BIN_DIR)/main.bin
	$(ESPTOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(BIN_DIR)/main.bin
	
$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h
	
$(BIN_DIR)/main.bin: $(SDK_OBJ) $(LIB_DIR)/lib_a-setjmp.o $(XS_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) 
	@echo "# ld main.bin"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.cpp
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(XS_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.cpp
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(LIB_DIR)/buildinfo.cpp -o $(LIB_DIR)/buildinfo.cpp.o
	$(LD) $(LD_FLAGS) -Wl,--start-group $^ $(LIB_DIR)/buildinfo.cpp $(LD_STD_LIBS) -Wl,--end-group -L$(LIB_DIR) -o $(TMP_DIR)/main.elf
	$(TOOLS_BIN)/xtensa-lx106-elf-objdump -t $(TMP_DIR)/main.elf > $(BIN_DIR)/main.sym
	$(ESPTOOL) -eo $(ARDUINO_ROOT)/bootloaders/eboot/eboot.elf -bo $@ -bm $(FLASH_MODE) -bf $(FLASH_SPEED) -bz $(FLASH_SIZE) -bs .text -bp 4096 -ec -eo $(TMP_DIR)/main.elf -bs .irom0.text -bs .text -bs .data -bs .rodata -bc -ec
	@echo "# Versions"
	@echo "#  ESP:   $(ESP_SDK_RELEASE)"
	@echo "#  XS:    $(XS_GIT_VERSION)"
	@$(TOOLS_BIN)/xtensa-lx106-elf-size -A $(TMP_DIR)/main.elf | perl -e $(MEM_USAGE)

$(LIB_DIR)/lib_a-setjmp.o: $(ESPRESSIF_SDK_ROOT)/lib/libcirom.a
	@echo "# ar" $(<F)
	(cd $(LIB_DIR) && $(AR) -xv $(ESPRESSIF_SDK_ROOT)/lib/libcirom.a lib_a-setjmp.o)

$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# cc" $(<F) "(strings in flash + force-l32)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 $< -o $@.unmapped
	$(TOOLS_BIN)/xtensa-lx106-elf-objcopy --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(LIB_DIR)/%.S.o: %.S
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $< -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(LIB_DIR)/%.cpp.o: %.cpp
	@echo "# cpp" $(<F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

	
$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)/xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp -r mc.resources.c
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

