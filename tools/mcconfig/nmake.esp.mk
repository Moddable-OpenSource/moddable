#
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

HOST_OS = win

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

ESP_SDK_DIR = $(BASE_DIR)\esp

ARDUINO_ROOT = $(ESP_SDK_DIR)\esp8266-2.3.0
TOOLS_ROOT = $(ESP_SDK_DIR)\xtensa-lx106-elf
SYSROOT = $(TOOLS_ROOT)\xtensa-lx106-elf\sysroot

PLATFORM_DIR = $(MODDABLE)\build\devices\esp

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\esp\release\lib
!ENDIF

# serial port configuration
UPLOAD_SPEED = 921600
!IF "$(UPLOAD_PORT)"==""
UPLOAD_PORT = com8
!ENDIF
UPLOAD_RESET = nodemcu
!IF "$(VERBOSE)"=="1"
UPLOAD_VERB = -v
!ENDIF

# Board settings for ESP-12E module (the most common); change for other modules
FLASH_SIZE = 4M
FLASH_MODE = dio
FLASH_SPEED = 40
FLASH_LAYOUT = eagle.flash.4m.ld

# WiFi & Debug settings
WIFI_SSID =
WIFI_PSK =

# End user-configurable values. Derived values below.
!IF "$(WIFI_SSID)"!=""
NET_CONFIG_FLAGS += -DWIFI_SSID=$(WIFI_SSID)
!ENDIF
!IF "$(WIFI_PSK)"!=""
NET_CONFIG_FLAGS += -DWIFI_PSK=$(WIFI_PSK)
!ENDIF

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

SDK = \
    -I$(PLATFORM_DIR)\lib\tinyprintf \
    -I$(PLATFORM_DIR)\lib\rtc

XS_OBJ = \
	$(LIB_DIR)\xsAll.o \
	$(LIB_DIR)\xsAPI.o \
	$(LIB_DIR)\xsArray.o \
	$(LIB_DIR)\xsAtomics.o \
	$(LIB_DIR)\xsBoolean.o \
	$(LIB_DIR)\xsCommon.o \
	$(LIB_DIR)\xsDataView.o \
	$(LIB_DIR)\xsDate.o \
	$(LIB_DIR)\xsDebug.o \
	$(LIB_DIR)\xsError.o \
	$(LIB_DIR)\xsFunction.o \
	$(LIB_DIR)\xsGenerator.o \
	$(LIB_DIR)\xsGlobal.o \
	$(LIB_DIR)\xsJSON.o \
	$(LIB_DIR)\xsMapSet.o \
	$(LIB_DIR)\xsMarshall.o \
	$(LIB_DIR)\xsMath.o \
	$(LIB_DIR)\xsMemory.o \
	$(LIB_DIR)\xsModule.o \
	$(LIB_DIR)\xsNumber.o \
	$(LIB_DIR)\xsObject.o \
	$(LIB_DIR)\xsPromise.o \
	$(LIB_DIR)\xsProperty.o \
	$(LIB_DIR)\xsProxy.o \
	$(LIB_DIR)\xsRegExp.o \
	$(LIB_DIR)\xsRun.o \
	$(LIB_DIR)\xsString.o \
	$(LIB_DIR)\xsSymbol.o \
	$(LIB_DIR)\xsType.o \
	$(LIB_DIR)\xsdtoa.o \
	$(LIB_DIR)\xspcre.o \
	$(LIB_DIR)\xsmc.o
XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\sources\pcre \
	-I$(XS_DIR)\platforms\esp \
	-I$(BUILD_DIR)\devices\esp
XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsesp.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
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
	$(CORE_DIR)\spiffs_hal.cpp \
	$(CORE_DIR)\StreamString.cpp \
	$(CORE_DIR)\Schedule.cpp \
	$(CORE_DIR)\time.c \
	$(CORE_DIR)\uart.c \
	$(CORE_DIR)\umm_malloc\umm_malloc.c \
	$(PLATFORM_DIR)\lib\fmod\e_fmod.c \
	$(PLATFORM_DIR)\lib\rtc\rtctime.c \
	$(PLATFORM_DIR)\lib\tinyprintf\tinyprintf.c

SDK_SRC_SKIPPED = \
	$(CORE_DIR)\base64.cpp \
	$(CORE_DIR)\cbuf.cpp \
	$(CORE_DIR)\core_esp8266_eboot_command.c \
	$(CORE_DIR)\core_esp8266_flash_utils.c \
	$(CORE_DIR)\core_esp8266_i2s.c \
	$(CORE_DIR)\core_esp8266_wiring_analog.c \
	$(CORE_DIR)\core_esp8266_wiring_pulse.c \
	$(CORE_DIR)\core_esp8266_wiring_shift.c \
	$(CORE_DIR)\debug.cpp \
	$(CORE_DIR)\pgmspace.cpp \
	$(CORE_DIR)\HardwareSerial.cpp \
	$(CORE_DIR)\IPAddress.cpp \
	$(CORE_DIR)\spiffs_api.cpp \
	$(CORE_DIR)\Print.cpp \
	$(CORE_DIR)\MD5Builder.cpp \
	$(CORE_DIR)\Stream.cpp \
	$(CORE_DIR)\Tone.cpp \
	$(CORE_DIR)\Updater.cpp \
	$(CORE_DIR)\WMath.cpp \
	$(CORE_DIR)\WString.cpp \
	$(CORE_DIR)\FS.cpp \
	$(CORE_DIR)\libb64\cdecode.c \
	$(CORE_DIR)\libb64\cencode.c

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
	$(LIB_DIR)\uart.o \
	$(LIB_DIR)\umm_malloc.o \
	$(LIB_DIR)\e_fmod.o \
	$(LIB_DIR)\rtctime.o \
	$(LIB_DIR)\tinyprintf.o \
	$(LIB_DIR)\Schedule.o \
	$(PLATFORM_DIR)\lib\fmod\e_fmod.c \

SDK_OBJ_SKIPPED = \
	$(LIB_DIR)\base64.o \
	$(LIB_DIR)\cbuf.o \
	$(LIB_DIR)\core_esp8266_eboot_command.o \
	$(LIB_DIR)\core_esp8266_flash_utils.o \
	$(LIB_DIR)\core_esp8266_i2s.o \
	$(LIB_DIR)\core_esp8266_wiring_analog.o \
	$(LIB_DIR)\core_esp8266_wiring_pulse.o \
	$(LIB_DIR)\core_esp8266_wiring_shift.o \
	$(LIB_DIR)\debug.o \
	$(LIB_DIR)\pgmspace.o \
	$(LIB_DIR)\HardwareSerial.o \
	$(LIB_DIR)\IPAddress.o \
	$(LIB_DIR)\spiffs_api.o \
	$(LIB_DIR)\Print.o \
	$(LIB_DIR)\MD5Builder.o \
	$(LIB_DIR)\Stream.o \
	$(LIB_DIR)\Tone.o \
	$(LIB_DIR)\Updater.o \
	$(LIB_DIR)\WMath.o \
	$(LIB_DIR)\WString.o \
	$(LIB_DIR)\FS.o \
	$(LIB_DIR)\cdecode.o \
	$(LIB_DIR)\cencode.o \
	$(LIB_DIR)\StreamString.o \
	$(LIB_DIR)\Schedule.o 

CPP_INCLUDES = \
	-I$(TOOLS_DIR)\xtensa-lx106-elf\include\c++\4.8.5

HEADERS = $(HEADERS) $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)\bin
CC  = $(TOOLS_BIN)\xtensa-lx106-elf-gcc
CPP = $(TOOLS_BIN)\xtensa-lx106-elf-g++
LD  = $(CPP)
AR  = $(TOOLS_BIN)\xtensa-lx106-elf-ar
ESPTOOL = $(ESP_SDK_DIR)\esptool.exe

AR_OPTIONS = rcs

BUILDCLUT = $(BUILD_DIR)\bin\win\release\buildclut
COMPRESSBMF = $(BUILD_DIR)\bin\win\release\compressbmf
RLE4ENCODE = $(BUILD_DIR)\bin\win\release\rle4encode
MCLOCAL = $(BUILD_DIR)\bin\win\release\mclocal
MCREZ = $(BUILD_DIR)\bin\win\release\mcrez
PNG2BMP = $(BUILD_DIR)\bin\win\release\png2bmp
IMAGE2CS = $(BUILD_DIR)\bin\win\release\image2cs
XSC = $(BUILD_DIR)\bin\win\release\xsc
XSID = $(BUILD_DIR)\bin\win\release\xsid
XSL = $(BUILD_DIR)\bin\win\release\xsl

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
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DmxNoConsole=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION)
!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ENDIF
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) -DMODINSTRUMENTATION=1 -DmxInstrument=1
!ENDIF
C_INCLUDES = $(DIRECTORIES) $(INC_DIRS) $(XS_DIRS) -I$(LIB_DIR) -I$(TMP_DIR)
C_FLAGS = -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99 -fdata-sections -ffunction-sections -fno-jump-tables
C_FLAGS_NODATASECTION = -c -Os -g -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99
CPP_FLAGS = -c -Os -g -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections
S_FLAGS = -c -g -x assembler-with-cpp -MMD
LD_FLAGS = -g -w -Os -nostdlib -Wl,-Map=$(BIN_DIR)\main.txt -Wl,--cref -Wl,--no-check-sections -u call_user_start -Wl,-static $(LD_DIRS) -T$(FLASH_LAYOUT) -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,register_chipv6_phy
LD_STD_LIBS = -lm -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig -lwps -lcrypto -laxtls -lstdc++
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

.PHONY: all	

APP_ARCHIVE = $(BIN_DIR)\libxsar.a
LIB_ARCHIVE = $(LIB_DIR)\libxslib.a

all: $(LAUNCH)

debug: $(LIB_DIR) $(LIB_ARCHIVE) $(APP_ARCHIVE) $(BIN_DIR)\main.bin
	tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
	$(ESPTOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(BIN_DIR)\main.bin
	$(BUILD_DIR)\bin\win\release\serial2xsbug $(UPLOAD_PORT) 460800 8N1 $(TMP_DIR)\main.elf

release: $(LIB_DIR) $(LIB_ARCHIVE) $(APP_ARCHIVE) $(BIN_DIR)\main.bin
	$(ESPTOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(BIN_DIR)\main.bin

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h

delAr:
	@del $(APP_ARCHIVE)
	@del $(LIB_ARCHIVE)

$(APP_ARCHIVE): $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsPlatform.o $(TMP_DIR)\main.o
	@echo # archive $(APP_ARCHIVE)
	$(AR) $(AR_OPTIONS) $@ $(TMP_DIR)\mc.xs.o $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsPlatform.o $(TMP_DIR)\mc.resources.o $(TMP_DIR)\main.o

$(LIB_ARCHIVE): $(XS_OBJ) $(SDK_OBJ)
	@echo # archive $(LIB_ARCHIVE)
#	$(AR) $(AR_OPTIONS) $@ $(TMP_DIR)\mc.xs.o $(LIB_DIR)\xsHost.o $(LIB_DIR)\xsPlatform.o $(TMP_DIR)\mc.resources.o $(LIB_DIR)\main.o

$(BIN_DIR)\main.bin: $(APP_ARCHIVE) $(LIB_ARCHIVE) $(LIB_DIR)\lib_a-setjmp.o
	@echo "# ld main.bin"
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.o
	$(LD) -L$(BIN_DIR) $(LD_FLAGS) -Wl,--start-group $(LIB_DIR)\buildinfo.o $(LIB_DIR)\lib_a-setjmp.o $(LD_STD_LIBS) -lxslib -lxsar -Wl,--end-group -L$(LIB_DIR) -o $(TMP_DIR)\main.elf
	$(TOOLS_BIN)\xtensa-lx106-elf-objdump -t $(TMP_DIR)\main.elf > $(BIN_DIR)\main.sym
	$(ESPTOOL) -eo $(ARDUINO_ROOT)\bootloaders\eboot\eboot.elf -bo $@ -bm $(FLASH_MODE) -bf $(FLASH_SPEED) -bz $(FLASH_SIZE) -bs .text -bp 4096 -ec -eo $(TMP_DIR)\main.elf -bs .irom0.text -bs .text -bs .data -bs .rodata -bc -ec

$(LIB_DIR)\lib_a-setjmp.o: $(SYSROOT)\lib\libcirom.a
	@echo "# ar" $?
	(cd $(LIB_DIR) && $(AR) -xv $(SYSROOT)\lib\libcirom.a lib_a-setjmp.o)

{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo "# cc - X1" $? "(strings in flash + (not) force-l32)"
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -mforce-l32 $? -o $@.unmapped
#	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .rodata.str1.1=.irom0.str.1 --rename-section .text=.irom0.code $@.unmapped $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\}.c{$(LIB_DIR)\}.o:
	@echo "# cc - X2" $?
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\}.cpp{$(LIB_DIR)\}.o:
	@echo "# cpp" $?
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_INCLUDES) $(CPP_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\cont.S.o: $(CORE_DIR)\cont.S
	@echo "# cc - X2" 
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $? -o $@
#	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .text=.iram1_0_seg $@.unmapped $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\libb64\}.c{$(LIB_DIR)\}.o:
	@echo "# cc - lib64" $?
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\spiffs\}.c{$(LIB_DIR)\}.o:
	@echo "# cc - spiffs" $?
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

{$(CORE_DIR)\umm_malloc\}.c{$(LIB_DIR)\}.o:
	@echo "# cc - umm_malloc" $?
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\e_fmod.o: $(PLATFORM_DIR)\lib\fmod\e_fmod.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\rtctime.o: $(PLATFORM_DIR)\lib\rtc\rtctime.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(LIB_DIR)\tinyprintf.o: $(PLATFORM_DIR)\lib\tinyprintf\tinyprintf.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(AR) $(AR_OPTIONS) $(LIB_ARCHIVE) $@

$(TMP_DIR)\xsHost.o: $(XS_DIR)\platforms\esp\xsHost.c
	@echo "# cc - " $?
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\xsPlatform.o: $(XS_DIR)\platforms\esp\xsPlatform.c
	@echo "# cc - " $?
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 $@.unmapped $@


$(TMP_DIR)\main.o: $(BUILD_DIR)\devices\esp\main.cpp
	@echo "# cc - " $?
	$(CPP) $? $(C_DEFINES) $(C_INCLUDES) $(CPP_INCLUDES) $(CPP_FLAGS) -o $@


$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)

$(TMP_DIR)\mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp -r mc.resources.c
	
$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	$(CC) $? $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
	$(TOOLS_BIN)\xtensa-lx106-elf-objcopy --rename-section .data=.irom0.str.1 --rename-section .rodata=.irom0.str.1 --rename-section .rodata.str1.1=.irom0.str.1 --rename-section .text=.irom0.text $@.unmapped $@


