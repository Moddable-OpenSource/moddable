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

HOST_OS = win

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

ESP_SDK_DIR = $(BASE_DIR)\esp
ESP_SDK_RELEASE = esp8266-2.3.0
ESPRESSIF_SDK_ROOT = $(ESP_SDK_DIR)\ESP8266_RTOS_SDK

ARDUINO_ROOT = $(ESP_SDK_DIR)\$(ESP_SDK_RELEASE)
TOOLS_ROOT = $(ESP_SDK_DIR)\xtensa-lx106-elf
SYSROOT = $(TOOLS_ROOT)\xtensa-lx106-elf\sysroot

PLATFORM_DIR = $(MODDABLE)\build\devices\esp

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

# serial port configuration
!IF "$(UPLOAD_SPEED)"==""
UPLOAD_SPEED = 921600
!ENDIF
!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 921600
!ENDIF
!IF "$(UPLOAD_PORT)"==""
UPLOAD_PORT = COM3
!ENDIF
UPLOAD_RESET = nodemcu
!IF "$(VERBOSE)"=="1"
UPLOAD_VERB = -v
!ENDIF

!IF "$(XSBUG_LOG)"=="1"
START_SERIAL2XSBUG= echo "Starting serial2xsbug..." && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && cd $(MODDABLE)\tools\xsbug-log && node xsbug-log $(BUILD_DIR)\bin\win\release\serial2xsbug $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -elf $(TMP_DIR)\main.elf
START_XSBUG=
!ELSE
START_SERIAL2XSBUG= echo "Starting serial2xsbug..." && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && $(BUILD_DIR)\bin\win\release\serial2xsbug $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -elf $(TMP_DIR)\main.elf
START_XSBUG= tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
!ENDIF
KILL_SERIAL2XSBUG= -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1


# Board settings for ESP-12E module (the most common); change for other modules
FLASH_SIZE = 4M
FLASH_MODE = qio
FLASH_SPEED = 80
FLASH_LAYOUT = eagle.flash.4m.ld

# WiFi & Debug settings
WIFI_SSID =
WIFI_PSK =

CORE_DIR = $(ARDUINO_ROOT)\cores\esp8266

INC_DIRS = \
	-I$(TOOLS_ROOT)\xtensa-lx106-elf\include\include \
	-I$(TOOLS_ROOT)\xtensa-lx106-elf\include \
	-I$(TOOLS_ROOT)\xtensa-lx106-elf\usr\include \
	-I$(SYSROOT)\usr\include \
	-I$(ARDUINO_ROOT)\tools\sdk\include \
	-I$(ARDUINO_ROOT)\tools\sdk\lwip\include \
	-I$(CORE_DIR) \
	-I$(ARDUINO_ROOT)\variants\generic \
	-I$(CORE_DIR)\spiffs \
	-I$(PLATFORM_DIR)\lib\rtc \
	-I$(PLATFORM_DIR)\lib\tinyprintf \
	-I$(PLATFORM_DIR)\lib\tinyi2s
SDK = \
    -I$(PLATFORM_DIR)\lib\tinyprintf \
    -I$(PLATFORM_DIR)\lib\rtc

XS_OBJ = \
	$(LIB_DIR)\xsAll.o \
	$(LIB_DIR)\xsAPI.o \
	$(LIB_DIR)\xsArguments.o \
	$(LIB_DIR)\xsArray.o \
	$(LIB_DIR)\xsAtomics.o \
	$(LIB_DIR)\xsBigInt.o \
	$(LIB_DIR)\xsBoolean.o \
	$(LIB_DIR)\xsCode.o \
	$(LIB_DIR)\xsCommon.o \
	$(LIB_DIR)\xsDataView.o \
	$(LIB_DIR)\xsDate.o \
	$(LIB_DIR)\xsDebug.o \
	$(LIB_DIR)\xsError.o \
	$(LIB_DIR)\xsFunction.o \
	$(LIB_DIR)\xsGenerator.o \
	$(LIB_DIR)\xsGlobal.o \
	$(LIB_DIR)\xsJSON.o \
	$(LIB_DIR)\xsLexical.o \
	$(LIB_DIR)\xsMapSet.o \
	$(LIB_DIR)\xsMarshall.o \
	$(LIB_DIR)\xsMath.o \
	$(LIB_DIR)\xsMemory.o \
	$(LIB_DIR)\xsModule.o \
	$(LIB_DIR)\xsNumber.o \
	$(LIB_DIR)\xsObject.o \
	$(LIB_DIR)\xsPlatforms.o \
	$(LIB_DIR)\xsPromise.o \
	$(LIB_DIR)\xsProperty.o \
	$(LIB_DIR)\xsProxy.o \
	$(LIB_DIR)\xsRegExp.o \
	$(LIB_DIR)\xsRun.o \
	$(LIB_DIR)\xsScope.o \
	$(LIB_DIR)\xsScript.o \
	$(LIB_DIR)\xsSourceMap.o \
	$(LIB_DIR)\xsString.o \
	$(LIB_DIR)\xsSymbol.o \
	$(LIB_DIR)\xsSyntaxical.o \
	$(LIB_DIR)\xsTree.o \
	$(LIB_DIR)\xsType.o \
	$(LIB_DIR)\xsdtoa.o \
	$(LIB_DIR)\xsmc.o \
	$(LIB_DIR)\xsre.o
XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\sources\pcre \
	-I$(XS_DIR)\platforms\esp \
	-I$(XS_DIR)\platforms\mc \
	-I$(BUILD_DIR)\devices\esp
XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsScript.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\esp\xsHost.h \
	$(XS_DIR)\platforms\mc\xsHosts.h \
	$(XS_DIR)\platforms\esp\xsPlatform.h
SDK_SRC = \
	$(CORE_DIR)\abi.cpp \
	$(CORE_DIR)\cont.S \
	$(CORE_DIR)\cont_util.c \
	$(CORE_DIR)\core_esp8266_main.cpp \
	$(CORE_DIR)\core_esp8266_noniso.c \
	$(CORE_DIR)\core_esp8266_phy.c \
	$(CORE_DIR)\core_esp8266_postmortem.c \
	$(CORE_DIR)\core_esp8266_si2c.c \
	$(CORE_DIR)\core_esp8266_timer.c \
	$(CORE_DIR)\core_esp8266_wiring.c \
	$(CORE_DIR)\core_esp8266_wiring_digital.c \
	$(CORE_DIR)\core_esp8266_wiring_pwm.c \
	$(CORE_DIR)\Esp.cpp \
	$(CORE_DIR)\heap.c \
	$(CORE_DIR)\libc_replacements.c \
	$(CORE_DIR)\spiffs\spiffs_cache.c \
	$(CORE_DIR)\spiffs\spiffs_check.c \
	$(CORE_DIR)\spiffs\spiffs_gc.c \
	$(CORE_DIR)\spiffs\spiffs_hydrogen.c \
	$(CORE_DIR)\spiffs\spiffs_nucleus.c \
	$(CORE_DIR)\StreamString.cpp \
	$(CORE_DIR)\Schedule.cpp \
	$(CORE_DIR)\time.c \
	$(CORE_DIR)\umm_malloc\umm_malloc.c \
	$(PLATFORM_DIR)\lib\bsearch\bsearch.c \
	$(PLATFORM_DIR)\lib\fmod\e_fmod.c \
	$(PLATFORM_DIR)\lib\rtc\rtctime.c \
	$(PLATFORM_DIR)\lib\tinyprintf\tinyprintf.c \
	$(PLATFORM_DIR)\lib\tinyuart\tinyuart.c \
	$(PLATFORM_DIR)\lib\tinyi2s\tinyi2s.c 

SDK_OBJ = \
	$(LIB_DIR)\abi.o \
	$(LIB_DIR)\cont.S.o \
	$(LIB_DIR)\cont_util.o \
	$(LIB_DIR)\core_esp8266_main.o \
	$(LIB_DIR)\core_esp8266_noniso.o \
	$(LIB_DIR)\core_esp8266_phy.o \
	$(LIB_DIR)\core_esp8266_postmortem.o \
	$(LIB_DIR)\core_esp8266_si2c.o \
	$(LIB_DIR)\core_esp8266_timer.o \
	$(LIB_DIR)\core_esp8266_wiring.o \
	$(LIB_DIR)\core_esp8266_wiring_digital.o \
	$(LIB_DIR)\core_esp8266_wiring_pwm.o \
	$(LIB_DIR)\Esp.o \
	$(LIB_DIR)\heap.o \
	$(LIB_DIR)\libc_replacements.o \
	$(LIB_DIR)\spiffs_cache.o \
	$(LIB_DIR)\spiffs_check.o \
	$(LIB_DIR)\spiffs_gc.o \
	$(LIB_DIR)\spiffs_hydrogen.o \
	$(LIB_DIR)\spiffs_nucleus.o \
	$(LIB_DIR)\spiffs_hal.o \
	$(LIB_DIR)\time.o \
	$(LIB_DIR)\umm_malloc.o \
	$(LIB_DIR)\bsearch.o \
	$(LIB_DIR)\e_fmod.o \
	$(LIB_DIR)\rtctime.o \
	$(LIB_DIR)\tinyprintf.o \
	$(LIB_DIR)\tinyuart.o \
	$(LIB_DIR)\tinyi2s.o \
	$(LIB_DIR)\Schedule.o \
	$(PLATFORM_DIR)\lib\fmod\e_fmod.c

CPP_INCLUDES = \
	-I$(TOOLS_DIR)\xtensa-lx106-elf\include\c++\4.8.5

HEADERS = $(HEADERS) $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)\bin
CC  = $(TOOLS_BIN)\xtensa-lx106-elf-gcc
CPP = $(TOOLS_BIN)\xtensa-lx106-elf-g++
LD  = $(CPP)
AR  = $(TOOLS_BIN)\xtensa-lx106-elf-ar
ESPTOOL = python $(ESPRESSIF_SDK_ROOT)\components\esptool_py\esptool\esptool.py

AR_OPTIONS = rcs

MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\release
BUILDCLUT = $(MODDABLE_TOOLS_DIR)\buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)\compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)\rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)\mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)\mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)\png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)\image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)\wav2maud
XSC = $(MODDABLE_TOOLS_DIR)\xsc
XSID = $(MODDABLE_TOOLS_DIR)\xsid
XSL = $(MODDABLE_TOOLS_DIR)\xsl

LD_DIRS = \
	-L$(MODDABLE)\build\devices\esp\sdk\ld\win \
	-L$(ARDUINO_ROOT)\tools\sdk\ld \
	-L$(ARDUINO_ROOT)\tools\sdk\lib \
	-L$(SYSROOT)\lib \
	-L$(SYSROOT)\usr\lib \

C_DEFINES = \
	-D__ets__ \
	-DICACHE_FLASH \
	-U__STRICT_ANSI__ \
	-DF_CPU=80000000L \
	-DARDUINO=10605 \
	-DARDUINO_ESP8266_ESP01 \
	-DARDUINO_ARCH_ESP8266 \
	-DWINBUILD=1 \
	-DESP8266 \
	-DCONT_STACKSIZE=4608 \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DmxNoConsole=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1 -DDEBUGGER_SPEED=$(DEBUGGER_SPEED)
!ENDIF
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) -DMODINSTRUMENTATION=1 -DmxInstrument=1
!ENDIF
!IF "$(ESP_SDK_RELEASE)"=="esp8266-2.3.0"
C_DEFINES = $(C_DEFINES) -DkESP8266Version=23
!ELSE
C_DEFINES = $(C_DEFINES) -DkESP8266Version=24
!ENDIF
C_INCLUDES = $(DIRECTORIES) $(INC_DIRS) $(XS_DIRS) -I$(LIB_DIR) -I$(TMP_DIR)
C_FLAGS = -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99 -fdata-sections -ffunction-sections -fno-jump-tables
C_FLAGS_NODATASECTION = -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99
CPP_FLAGS = -c -Os -g -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections
S_FLAGS = -c -g -x assembler-with-cpp -MMD

!IF "$(ESP_SDK_RELEASE)"=="esp8266-2.4.0"
LD_FLAGS = -g -w -Os -nostdlib -Wl,-Map=$(BIN_DIR)\main.txt -Wl,--cref -Wl,--no-check-sections -u call_user_start -Wl,-static $(LD_DIRS) -T$(FLASH_LAYOUT) -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,spi_flash_read
LD_STD_LIBS = -lm -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lc -lcrypto
!ELSE
LD_FLAGS = -g -w -Os -nostdlib -Wl,-Map=$(BIN_DIR)\main.txt -Wl,--cref -Wl,--no-check-sections -u call_user_start -Wl,-static $(LD_DIRS) -T$(FLASH_LAYOUT) -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,register_chipv6_phy
LD_STD_LIBS = -lm -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig -lwps -lcrypto -laxtls -lstdc++
!ENDIF

# LD_STD_LIBS = -lm -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig -lwps -lcrypto -lmbedtls -lg -lcirom -lfreertos -lmirom
# LD_STD_LIBS = -lm -lhal -lphy -lnet80211 -llwip -lwpa -lpp -lsmartconfig -lwps -lcrypto -lmbedtls -lmirom -lgcc -lg -lmain
# stdc++ used in later versions of esp8266 Arduino
# LD_STD_CPP = lstdc++

# ifneq ($(shell grep $(LD_STD_CPP) $(ARDUINO_ROOT)\platform.txt),)
# 	LD_STD_LIBS += -$(LD_STD_CPP)
# endif

# VPATH += $(SDK_DIRS) $(XS_DIRS)
#
!IF "$(DEBUG)"=="1"
LAUNCH = debug
!ELSE
LAUNCH = release
!ENDIF

ESP_FIRMWARE_DIR = $(ESPRESSIF_SDK_ROOT)\components\esp8266\firmware
ESP_BOOTLOADER_BIN = $(ESP_FIRMWARE_DIR)\boot_v1.7.bin
ESP_DATA_DEFAULT_BIN = $(ESP_FIRMWARE_DIR)\esp_init_data_default.bin

!IF "$(FLASH_SIZE)"=="1M"
ESP_INIT_DATA_DEFAULT_BIN_OFFSET = 0xFC000
!ELSEIF "$(FLASH_SIZE)"=="4M"
ESP_INIT_DATA_DEFAULT_BIN_OFFSET = 0x3FC000
!ENDIF

ESPTOOL_FLASH_OPT = \
	--flash_freq $(FLASH_SPEED)m \
	--flash_mode $(FLASH_MODE) \
	--flash_size $(FLASH_SIZE)B \
	0x0000 $(ESP_BOOTLOADER_BIN) \
	0x1000 $(BIN_DIR)\main.bin \
	$(ESP_INIT_DATA_DEFAULT_BIN_OFFSET) $(ESP_DATA_DEFAULT_BIN)

UPLOAD_TO_ESP = $(ESPTOOL) -b $(UPLOAD_SPEED) -p $(UPLOAD_PORT) write_flash $(ESPTOOL_FLASH_OPT)

.PHONY: all	

APP_ARCHIVE = $(BIN_DIR)\libxsar.a
LIB_ARCHIVE = $(LIB_DIR)\libxslib.a

all: $(LAUNCH)

clean:
	$(KILL_SERIAL2XSBUG)
	echo # Clean project bin and tmp
	echo $(BIN_DIR)
	del /s/q/f $(BIN_DIR)\*.* > NUL
	rmdir /s/q $(BIN_DIR)
	echo $(TMP_DIR)
	del /s/q/f $(TMP_DIR)\*.* > NUL
	rmdir /s/q $(TMP_DIR)
	echo $(LIB_DIR)
	del /s/q/f $(LIB_DIR)\*.* > NUL
	rmdir /s/q $(LIB_DIR)

precursor: $(LIB_DIR) $(LIB_ARCHIVE) $(APP_ARCHIVE) $(BIN_DIR)\main.bin
	

debug: precursor
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(UPLOAD_TO_ESP)
	$(START_SERIAL2XSBUG)

release: precursor
	$(KILL_SERIAL2XSBUG)
	$(UPLOAD_TO_ESP)

build: precursor

deploy:
	$(KILL_SERIAL2XSBUG)
	if not exist $(BIN_DIR)\main.bin (echo # Build before deploy) else ( $(UPLOAD_TO_ESP) )

xsbug:
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(START_SERIAL2XSBUG)

$(LIB_DIR)\buildinfo.h:
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h

delAr:
	@del $(APP_ARCHIVE)
	@del $(LIB_ARCHIVE)

$(APP_ARCHIVE): $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsHosts.o $(TMP_DIR)\xsPlatform.o $(TMP_DIR)\main.o
	@echo # archive $(APP_ARCHIVE)
	$(AR) $(AR_OPTIONS) $@ $(TMP_DIR)\mc.xs.o $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsHosts.o $(TMP_DIR)\xsPlatform.o $(TMP_DIR)\mc.resources.o $(TMP_DIR)\main.o

$(LIB_ARCHIVE): $(XS_OBJ) $(SDK_OBJ)
	@echo # archive $(LIB_ARCHIVE)
#	$(AR) $(AR_OPTIONS) $@ $(TMP_DIR)\mc.xs.o $(LIB_DIR)\xsHost.o $(LIB_DIR)\xsPlatform.o $(TMP_DIR)\mc.resources.o $(LIB_DIR)\main.o

$(BIN_DIR)\main.bin: $(APP_ARCHIVE) $(LIB_ARCHIVE) $(LIB_DIR)\lib_a-setjmp.o $(LIB_DIR)\buildinfo.h
	@echo # ld main.bin
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.o
	$(LD) -L$(BIN_DIR) $(LD_FLAGS) -Wl,--start-group $(LIB_DIR)\buildinfo.o $(LIB_DIR)\lib_a-setjmp.o $(LD_STD_LIBS) -lxslib -lxsar -Wl,--end-group -L$(LIB_DIR) -o $(TMP_DIR)\main.elf
	$(TOOLS_BIN)\xtensa-lx106-elf-objdump -t $(TMP_DIR)\main.elf > $(BIN_DIR)\main.sym
	$(ESPTOOL) --chip esp8266 elf2image --version=2 -o $@ $(TMP_DIR)\main.elf

$(LIB_DIR)\lib_a-setjmp.o: $(SYSROOT)\lib\libcirom.a
	@echo # ar $(@F)
	(cd $(LIB_DIR) && $(AR) -xv $(SYSROOT)\lib\libcirom.a lib_a-setjmp.o)

{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash + (not) force-l32)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 $? -o $@.unmapped
#	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .rodata.str1.1=.irom0.str.1 --rename-section .text=.irom0.code $@.unmapped $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\}.cpp{$(LIB_DIR)\}.o:
	@echo # cpp $(@F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_INCLUDES) $(CPP_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\cont.S.o: $(CORE_DIR)\cont.S
	@echo # cc cont.S
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $? -o $@
#	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .text=.iram1_0_seg $@.unmapped $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\libb64\}.c{$(LIB_DIR)\}.o:
	@echo # cc lib64 $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\spiffs\}.c{$(LIB_DIR)\}.o:
	@echo # cc spiffs $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\umm_malloc\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\bsearch.o: $(PLATFORM_DIR)\lib\bsearch\bsearch.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\e_fmod.o: $(PLATFORM_DIR)\lib\fmod\e_fmod.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\rtctime.o: $(PLATFORM_DIR)\lib\rtc\rtctime.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\tinyprintf.o: $(PLATFORM_DIR)\lib\tinyprintf\tinyprintf.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\tinyuart.o: $(PLATFORM_DIR)\lib\tinyuart\tinyuart.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\tinyi2s.o: $(PLATFORM_DIR)\lib\tinyi2s\tinyi2s.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(TMP_DIR)\xsHost.o: $(XS_DIR)\platforms\esp\xsHost.c
	@echo # cc $(@F)
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\xsHosts.o: $(XS_DIR)\platforms\mc\xsHosts.c
	@echo # cc $(@F)
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\xsPlatform.o: $(XS_DIR)\platforms\esp\xsPlatform.c
	@echo # cc $(@F)
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\main.o: $(BUILD_DIR)\devices\esp\main.cpp
	@echo # cc $(@F)
	$(CPP) $? $(C_DEFINES) $(C_INCLUDES) $(CPP_INCLUDES) $(CPP_FLAGS) -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)

$(TMP_DIR)\mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -p esp -r mc.resources.c
	
$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 --rename-section .text=.irom0.text $@.unmapped $@


