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

!IF "$(IDF_PATH)"==""
!MESSAGE %IDF_PATH% not set. See set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/esp32.md
!ERROR
!ENDIF

!IF "$(EXPECTED_ESP_IDF)"==""
EXPECTED_ESP_IDF = v4.4.3
!ENDIF

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
CMAKE_LOG_LEVEL = VERBOSE
IDF_PY_LOG_FLAG = -v
!ELSE
!CMDSWITCHES +S
CMAKE_LOG_LEVEL = ERROR
IDF_PY_LOG_FLAG = -n
!ENDIF

!IF "$(ESP32_SUBCLASS)"==""
ESP32_SUBCLASS = esp32
!ENDIF

!IF "$(ESP32_SUBCLASS)"=="esp32c3"
ESP_ARCH = riscv
!ELSE
ESP_ARCH = xtensa
!ENDIF

!IF "$(UPLOAD_SPEED)"==""
UPLOAD_SPEED = 921600
!ENDIF
!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 460800
!ENDIF

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

!IF [cd /D $(IDF_PATH) && git describe --always --abbrev=0 > $(TMP_DIR)\_idf_version.tmp 2> nul] == 0
IDF_VERSION = \
!INCLUDE $(TMP_DIR)\_idf_version.tmp
!IF [del $(TMP_DIR)\_idf_version.tmp] == 0
!ENDIF
!ELSE
!MESSAGE Could not detect ESP-IDF version.
!ENDIF

!IF "$(IDF_VERSION)"==""
!MESSAGE Could not detect ESP-IDF version at %IDF_PATH%: $(IDF_PATH).
!ERROR
!ENDIF

PROJ_DIR_TEMPLATE = $(BUILD_DIR)\devices\esp32\xsProj-$(ESP32_SUBCLASS)

!IF "$(UPLOAD_PORT)"==""
!IF [python $(PROJ_DIR_TEMPLATE)\getPort.py $(IDF_PATH)\tools > $(TMP_DIR)\_default_port.tmp 2> nul] == 0
DEFAULT_PORT = \
!INCLUDE $(TMP_DIR)\_default_port.tmp
!IF [del $(TMP_DIR)\_default_port.tmp] == 0
!ENDIF
!ENDIF
PORT_TO_USE = $(DEFAULT_PORT)
PORT_COMMAND = 
!ELSE
PORT_TO_USE = $(UPLOAD_PORT)
PORT_COMMAND = -p $(UPLOAD_PORT)
!ENDIF

PROJ_DIR = $(TMP_DIR)\xsProj-$(ESP32_SUBCLASS)

!IF "$(DEBUG)"=="1"
START_XSBUG= tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
BUILD_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) build -D mxDebug=1 -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS)
BUILD_MSG =
DEPLOY_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) flash -D mxDebug=1 -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS)
START_SERIAL2XSBUG = echo Launching app... & echo Type Ctrl-C twice after debugging app. & set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && $(BUILD_DIR)\bin\win\release\serial2xsbug $(PORT_TO_USE) $(DEBUGGER_SPEED) 8N1
!ELSE
START_XSBUG=
START_SERIAL2XSBUG = echo No debugger for a release build.
BUILD_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) build -D mxDebug=0 -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS)
DEPLOY_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) flash -D mxDebug=0 -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS)

!ENDIF
KILL_SERIAL2XSBUG= -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1

!IF "$(XSBUG_LOG)"=="1"
START_SERIAL2XSBUG = echo Launching app... & set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && cd $(MODDABLE)\tools\xsbug-log && node xsbug-log start /B $(BUILD_DIR)\bin\win\release\serial2xsbug $(PORT_TO_USE) $(DEBUGGER_SPEED) 8N1 
START_XSBUG =
!ENDIF

IDF_RECONFIGURE_CMD=python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) reconfigure -D SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D IDF_TARGET=$(ESP32_SUBCLASS) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) -D SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE)

BLD_DIR = $(PROJ_DIR)\build

PLATFORM_DIR = $(BUILD_DIR)\devices\esp32

INC_DIRS = \
 	-I$(IDF_PATH)\components \
 	-I$(IDF_PATH)\components\bootloader_support\include \
 	-I$(IDF_PATH)\components\bt\include \
	-I$(IDF_PATH)\components\bt\include\$(ESP32_SUBCLASS)\include \
 	-I$(IDF_PATH)\components\bt\host\bluedroid\api\include \
 	-I$(IDF_PATH)\components\bt\host\bluedroid\api\include\api \
 	-I$(IDF_PATH)\components\driver\include \
	-I$(IDF_PATH)\components\driver\include\driver \
	-I$(IDF_PATH)\components\driver\$(ESP32_SUBCLASS)\include \
	-I$(IDF_PATH)\components\driver\$(ESP32_SUBCLASS)\include\driver \
	-I$(IDF_PATH)\components\esp_common\include \
 	-I$(IDF_PATH)\components\$(ESP32_SUBCLASS)\include \
	-I$(IDF_PATH)\components\$(ESP32_SUBCLASS) \
 	-I$(IDF_PATH)\components\esp_event\include \
	-I$(IDF_PATH)\components\esp_eth\include \
	-I$(IDF_PATH)\components\esp_hw_support\include \
	-I$(IDF_PATH)\components\esp_hw_support\include\soc \
 	-I$(IDF_PATH)\components\esp_netif\include \
 	-I$(IDF_PATH)\components\esp_pm\include \
 	-I$(IDF_PATH)\components\esp_ringbuf\include \
	-I$(IDF_PATH)\components\esp_rom\include \
 	-I$(IDF_PATH)\components\esp_rom\include\$(ESP32_SUBCLASS) \
 	-I$(IDF_PATH)\components\esp_system\include \
 	-I$(IDF_PATH)\components\esp_timer\include \
 	-I$(IDF_PATH)\components\esp_wifi\include \
 	-I$(IDF_PATH)\components\$(ESP_ARCH)\include \
	-I$(IDF_PATH)\components\$(ESP_ARCH)\$(ESP32_SUBCLASS)\include \
 	-I$(IDF_PATH)\components\freertos \
 	-I$(IDF_PATH)\components\freertos\include \
 	-I$(IDF_PATH)\components\freertos\include\freertos \
	-I$(IDF_PATH)\components\freertos\port \
 	-I$(IDF_PATH)\components\freertos\port\$(ESP_ARCH)\include \
	-I$(IDF_PATH)\components\freertos\port\$(ESP_ARCH)\include\freertos \
	-I$(IDF_PATH)\components\freertos\include\esp_additions \
	-I$(IDF_PATH)\components\freertos\include\esp_additions\freertos \
	-I$(IDF_PATH)\components\hal\include \
	-I$(IDF_PATH)\components\hal\$(ESP32_SUBCLASS)\include \
	-I$(IDF_PATH)\components\hal\platform_port\include \
	-I$(IDF_PATH)\components\heap\include \
	-I$(IDF_PATH)\components\log\include \
	-I$(IDF_PATH)\components\lwip\include\apps \
	-I$(IDF_PATH)\components\lwip\include\apps\sntp \
	-I$(IDF_PATH)\components\lwip\lwip\src\include \
	-I$(IDF_PATH)\components\lwip\port\esp32\include \
	-I$(IDF_PATH)\components\mbedtls\include \
	-I$(IDF_PATH)\components\newlib\include \
	-I$(IDF_PATH)\components\newlib\platform_include \
	-I$(IDF_PATH)\components\bt\host\nimble\esp-hci\include \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\nimble\host\include \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\nimble\host\services\gap\include \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\nimble\host\src \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\nimble\include \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\nimble\include\nimble \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\porting\nimble\include \
	-I$(IDF_PATH)\components\bt\host\nimble\nimble\porting\npl\freertos\include \
	-I$(IDF_PATH)\components\bt\host\nimble\port\include \
	-I$(IDF_PATH)\components\soc\$(ESP32_SUBCLASS)\include \
	-I$(IDF_PATH)\components\soc\$(ESP32_SUBCLASS)\include\soc \
	-I$(IDF_PATH)\components\soc\include \
	-I$(IDF_PATH)\components\soc\include\soc \
	-I$(IDF_PATH)\components\spiffs\include \
	-I$(IDF_PATH)\components\fatfs\src \
	-I$(IDF_PATH)\components\fatfs\vfs \
	-I$(IDF_PATH)\components\wear_levelling\include \
	-I$(IDF_PATH)\components\sdmmc\include \
	-I$(IDF_PATH)\components\spi_flash\include \
	-I$(IDF_PATH)\components\tcpip_adapter\include \
	-I$(IDF_PATH)\components\tcpip_adapter \
	-I$(IDF_PATH)\components\vfs\include

XS_OBJ = \
	$(LIB_DIR)\xsHost.o \
	$(LIB_DIR)\xsHosts.o \
	$(LIB_DIR)\xsPlatform.o \
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
	$(LIB_DIR)\xsre.o \
	$(LIB_DIR)\xsmc.o \
	$(LIB_DIR)\e_pow.o

SDKCONFIG_H_DIR = $(BLD_DIR)\config

!IF "$(ESP32_SUBCLASS)"=="esp32c3"
ESP32_TARGET = 4
!ELSEIF "$(ESP32_SUBCLASS)"=="esp32s3"
ESP32_TARGET = 3
!ELSEIF "$(ESP32_SUBCLASS)"=="esp32s2"
ESP32_TARGET = 2
!ELSE
ESP32_TARGET = 1
!ENDIF

XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\platforms\esp \
	-I$(XS_DIR)\platforms\mc \
	-I$(SDKCONFIG_H_DIR) \
	-I$(PLATFORM_DIR)\lib\pow

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\esp\xsHost.h \
	$(XS_DIR)\platforms\mc\xsHosts.h \
	$(XS_DIR)\platforms\esp\xsPlatform.h

!IF "$(SDKCONFIGPATH)"==""
SDKCONFIGPATH = $(PROJ_DIR)
!ENDIF
SDKCONFIG = $(SDKCONFIGPATH)\sdkconfig.defaults
SDKCONFIGPRIOR = $(SDKCONFIGPATH)\sdkconfig.defaults.prior
SDKCONFIG_H = $(SDKCONFIG_H_DIR)\sdkconfig.h

HEADERS = $(HEADERS) $(XS_HEADERS)

TOOLS_BIN = 
!IF "$(ESP32_SUBCLASS)"=="esp32c3"
CC = $(TOOLS_BIN)riscv32-esp-elf-gcc
CPP = $(TOOLS_BIN)riscv32-esp-elf-g++
LD = $(CPP)
AR = $(TOOLS_BIN)riscv32-esp-elf-ar
OBJCOPY = $(TOOLS_BIN)riscv32-esp-elf-objcopy
!ELSE
CC = $(TOOLS_BIN)xtensa-$(ESP32_SUBCLASS)-elf-gcc
CPP = $(TOOLS_BIN)xtensa-$(ESP32_SUBCLASS)-elf-g++
LD = $(CPP)
AR = $(TOOLS_BIN)xtensa-$(ESP32_SUBCLASS)-elf-ar
OBJCOPY = $(TOOLS_BIN)xtensa-$(ESP32_SUBCLASS)-elf-objcopy
!ENDIF

AR_OPTIONS = crs

MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\release
BUILDCLUT = $(MODDABLE_TOOLS_DIR)\buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)\compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)\rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)\mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)\mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)\png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)\image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)\wav2maud
BLES2GATT = $(MODDABLE_TOOLS_DIR)\bles2gatt
XSC = $(MODDABLE_TOOLS_DIR)\xsc
XSID = $(MODDABLE_TOOLS_DIR)\xsid
XSL = $(MODDABLE_TOOLS_DIR)\xsl

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

!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ENDIF
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) -DMODINSTRUMENTATION=1 -DmxInstrument=1
!ENDIF
C_INCLUDES = $(C_INCLUDES) $(DIRECTORIES) $(INC_DIRS) $(XS_DIRS) -I$(LIB_DIR) -I$(TMP_DIR)

C_COMMON_FLAGS = -c -Os -g \
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

!IF "$(ESP_ARCH)"=="riscv"
C_COMMON_FLAGS = $(C_COMMON_FLAGS) \
	-march=rv32imc
!ELSE
C_COMMON_FLAGS = $(C_COMMON_FLAGS) \
	-mlongcalls \
	-mtext-section-literals \
!ENDIF

C_FLAGS = $(C_COMMON_FLAGS) \
	-Wno-implicit-function-declaration \
	-std=gnu99 \
	$(C_FLAGS_SUBPLATFORM)

CPP_FLAGS = $(C_COMMON_FLAGS)

!IF "$(DEBUG)"=="1"
LAUNCH = debug
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ELSE
LAUNCH = release
!ENDIF

PARTITIONS_BIN = partition-table.bin
PARTITIONS_PATH = $(BLD_DIR)\partition_table\$(PARTITIONS_BIN)

PROJ_DIR_FILES = \
	$(PROJ_DIR)\main\main.c	\
	$(PROJ_DIR)\main\component.mk	\
	$(PROJ_DIR)\main\CMakeLists.txt \
	$(PROJ_DIR)\CMakeLists.txt \
	$(PROJ_DIR)\partitions.csv \
	$(PROJ_DIR)\Makefile

!IF "$(BOOTLOADERPATH)"!=""
!IF [fc $(BOOTLOADERPATH)\subproject\main\bootloader_start.c $(PROJ_DIR)\components\bootloader\subproject\main\bootloader_start.c > nul 2> nul] != 0
!IF [rmdir /S /Q $(PROJ_DIR)\bootloader > nul 2> nul] == 0
!ENDIF
!ENDIF
PROJ_DIR_FILES = $(PROJ_DIR_FILES) \
	$(PROJ_DIR)\components\bootloader\subproject\main\bootloader_start.c
!ELSE
!IF [rmdir /S /Q $(PROJ_DIR)\components > nul 2> nul && rmdir /S /Q $(PROJ_DIR)\bootloader > nul 2> nul] == 1
!ENDIF
!ENDIF

.PHONY: all

all: $(LAUNCH)

clean:
	echo # Clean project lib, bin and tmp
	echo $(BIN_DIR)
	del /s/q/f $(BIN_DIR)\*.* > NUL
	rmdir /s/q $(BIN_DIR)
	echo $(TMP_DIR)
	del /s/q/f $(TMP_DIR)\*.* > NUL
	rmdir /s/q $(TMP_DIR)
	echo $(LIB_DIR)
	if exist $(LIB_DIR) del /s/q/f $(LIB_DIR)\*.* > NUL
	if exist $(LIB_DIR) rmdir /s/q $(LIB_DIR)
	echo $(PROJ_DIR)
	if exist $(PROJ_DIR) del /s/q/f $(PROJ_DIR)\*.* > NUL
	if exist $(PROJ_DIR) rmdir /s/q $(PROJ_DIR)

precursor: idfVersionCheck $(BLE) $(SDKCONFIG_H) $(LIB_DIR) $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a
	copy $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a $(BLD_DIR)\.

debug: precursor
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	copy $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a $(BLD_DIR)\.
	-cd $(PROJ_DIR) & python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) build flash -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D mxDebug=1 -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) -D SDKCONFIG_DEFAULTS="$(SDKCONFIG_FILE)"
	-copy $(BLD_DIR)\xs_esp32.map $(BIN_DIR)\. > nul 2>&1
	-copy $(BLD_DIR)\xs_esp32.bin $(BIN_DIR)\. > nul 2>&1
	-copy $(BLD_DIR)\partition_table\partition-table.bin $(BIN_DIR)\. > nul 2>&1
	-copy $(BLD_DIR)\bootloader\bootloader.bin $(BIN_DIR)\. > nul 2>&1
	-copy $(PARTITIONS_PATH) $(BIN_DIR)\. > nul 2>&1
	-copy $(BLD_DIR)\ota_data_initial.bin $(BIN_DIR)\. > nul 2>&1
	$(START_SERIAL2XSBUG)

release: precursor
	$(KILL_SERIAL2XSBUG)
	if exist $(BLD_DIR)\xs_esp32.elf del $(BLD_DIR)\xs_esp32.elf
	if not exist $(BLD_DIR) mkdir $(BLD_DIR)
	copy $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a $(BLD_DIR)\.
	cd $(PROJ_DIR) & python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) build flash -D INSTRUMENT=$(INSTRUMENT) -D TMP_DIR="$(TMP_DIR)" -D mxDebug=0 -D SDKCONFIG_HEADER="$(SDKCONFIG_H)" -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) -D DEBUGGER_SPEED=$(DEBUGGER_SPEED) -D ESP32_SUBCLASS=$(ESP32_SUBCLASS) -D SDK_CONFIG_DEFAULTS=$(SDKCONFIG_FILE)
	copy $(BLD_DIR)\xs_esp32.map $(BIN_DIR)\.
	copy $(BLD_DIR)\xs_esp32.bin $(BIN_DIR)\.
	copy $(BLD_DIR)\partition_table\partition-table.bin $(BIN_DIR)
	copy $(BLD_DIR)\bootloader\bootloader.bin $(BIN_DIR)\.
	python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) monitor

prepare:
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	if exist $(BLD_DIR)\xs_esp32.elf del $(BLD_DIR)\xs_esp32.elf
	if not exist $(BLD_DIR) mkdir $(BLD_DIR)
	copy $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a $(BLD_DIR)\.
	set HOME=$(PROJ_DIR)
	cd $(PROJ_DIR)
		
build: precursor prepare
	echo $(BUILD_CMD)
	$(BUILD_CMD)
	$(BUILD_MSG)
	copy $(BLD_DIR)\bootloader\bootloader.bin $(BIN_DIR)
	copy $(BLD_DIR)\partition_table\partition-table.bin $(BIN_DIR)
	if exist $(BLD_DIR)\ota_data_initial.bin copy $(BLD_DIR)\ota_data_initial.bin $(BIN_DIR)
	copy $(BLD_DIR)\xs_esp32.bin $(BIN_DIR)
	copy $(BLD_DIR)\xs_esp32.map $(BIN_DIR)

xsbug:
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(START_SERIAL2XSBUG)

DEPLOY_PRE:
	$(KILL_SERIAL2XSBUG)
	if not exist $(BIN_DIR)\xs_esp32.bin echo "Please build before deploy"
	if not exist $(BIN_DIR)\xs_esp32.bin exit 1
	if exist $(BLD_DIR)\xs_esp32.bin move /Y $(BLD_DIR)\xs_esp32.bin $(BLD_DIR)\xs_esp32.bin_prev
	if exist $(PARTITIONS_PATH) move /Y $(PARTITIONS_PATH) $(PARTITIONS_PATH)_prev
	if exist $(BLD_DIR)\bootloader\bootloader.bin move /Y $(BLD_DIR)\bootloader\bootloader.bin $(BLD_DIR)\bootloader\bootloader.bin_prev
	if exist $(BLD_DIR)\ota_data_initial.bin move /Y $(BLD_DIR)\ota_data_initial.bin $(BLD_DIR)\ota_data_initial.bin_prev

DEPLOY_START:
	if exist $(BIN_DIR)\xs_esp32.bin copy $(BIN_DIR)\xs_esp32.bin $(BLD_DIR)
	if exist $(BIN_DIR)\$(PARTITIONS_BIN) copy $(BIN_DIR)\$(PARTITIONS_BIN) $(PARTITIONS_PATH)
	if exist $(BIN_DIR)\bootloader.bin  copy $(BIN_DIR)\bootloader.bin $(BLD_DIR)\bootloader\bootloader.bin
	if exist $(BIN_DIR)\ota_data_initial.bin copy $(BIN_DIR)\ota_data_initial.bin $(BLD_DIR)\ota_data_initial.bin
	set HOME=$(PROJ_DIR)
	cd $(PROJ_DIR)
	echo $(DEPLOY_CMD)
	$(DEPLOY_CMD)

DEPLOY_END:
	if exist $(BLD_DIR)\xs_esp32.bin del $(BLD_DIR)\xs_esp32.bin
	if exist $(PARTITIONS_PATH) del $(PARTITIONS_PATH)
	if exist $(BLD_DIR)\bootloader\bootloader.bin del $(BLD_DIR)\bootloader\bootloader.bin
	if exist $(BLD_DIR)\ota_data_initial.bin del $(BLD_DIR)\ota_data_initial.bin
	if exist $(BLD_DIR)\xs_esp32.bin_prev move /Y $(BLD_DIR)\xs_esp32.bin_prev $(BLD_DIR)\xs_esp32.bin
	if exist $(PARTITIONS_PATH)_prev move /Y $(PARTITIONS_PATH)_prev $(PARTITIONS_PATH)
	if exist $(BLD_DIR)\bootloader\bootloader.bin_prev move /Y $(BLD_DIR)\bootloader\bootloader.bin_prev $(BLD_DIR)\bootloader\bootloader.bin
	if exist $(BLD_DIR)\ota_data_initial.bin_prev move /Y $(BLD_DIR)\ota_data_initial.bin_prev $(BLD_DIR)\ota_data_initial.bin

deploy: DEPLOY_PRE DEPLOY_START DEPLOY_END

idfVersionCheck:
	python $(PROJ_DIR_TEMPLATE)\versionCheck.py $(EXPECTED_ESP_IDF) $(IDF_VERSION) || (echo "Expected ESP IDF $(EXPECTED_ESP_IDF), found $(IDF_VERSION)" && exit 1)

xidfVersionCheck:
	python $(PROJ_DIR_TEMPLATE)\versionCheck.py $(EXPECTED_ESP_IDF) $(IDF_VERSION)
	if %ERRORLEVEL% NEQ 0 (
		echo "Expected ESP-IDF $(EXPECTED_ESP_IDF), found $(IDF_VERSION)"
		exit 1
	)

$(SDKCONFIG_H): $(SDKCONFIG_FILE) $(PROJ_DIR_FILES)
	@echo Reconfiguring ESP-IDF...
	if exist $(PROJ_DIR)\sdkconfig del $(PROJ_DIR)\sdkconfig
	cd $(PROJ_DIR) 
	$(IDF_RECONFIGURE_CMD)
	COPY /B $(SDKCONFIG_H)+,,


$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

$(BIN_DIR)\xs_$(ESP32_SUBCLASS).a: $(PROJ_DIR)\main\main.c $(SDKCONFIG_H) $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS)
	@echo # ld xs_esp32.bin
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(TMP_DIR)\buildinfo.h
	echo #include "buildinfo.h" > $(TMP_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(TMP_DIR)\buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(TMP_DIR)\buildinfo.c -o $(TMP_DIR)\buildinfo.c.o
	$(AR) $(AR_OPTIONS) $(BIN_DIR)\xs_$(ESP32_SUBCLASS).a $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(TMP_DIR)\buildinfo.c.o

$(PROJ_DIR) : $(PROJ_DIR_TEMPLATE)
	echo d | xcopy /s $(PROJ_DIR_TEMPLATE)\* $(PROJ_DIR)

$(PROJ_DIR)\main:
	mkdir $(PROJ_DIR)\main

$(PROJ_DIR)\main\main.c: $(PROJ_DIR)\main $(PROJ_DIR_TEMPLATE)\main\main.c
	copy $(PROJ_DIR_TEMPLATE)\main\main.c $@

$(PROJ_DIR)\main\component.mk: $(PROJ_DIR)\main $(PROJ_DIR_TEMPLATE)\main\component.mk
	copy $(PROJ_DIR_TEMPLATE)\main\component.mk $@

$(PROJ_DIR)\main\CMakeLists.txt: $(PROJ_DIR)\main $(PROJ_DIR_TEMPLATE)\main\CMakeLists.txt
	copy $(PROJ_DIR_TEMPLATE)\main\CMakeLists.txt $@

$(PROJ_DIR)\CMakeLists.txt: $(PROJ_DIR_TEMPLATE)\CMakeLists.txt
	copy $(PROJ_DIR_TEMPLATE)\CMakeLists.txt $@

$(PROJ_DIR)\Makefile: $(PROJ_DIR_TEMPLATE)\Makefile
	copy $? $@

$(PROJ_DIR)\components\bootloader\subproject\main\bootloader_start.c: $(PROJ_DIR) $(BOOTLOADERPATH)\subproject\main\bootloader_start.c
	echo Using custom bootloader: $(BOOTLOADERPATH)
	xcopy /s /y $(BOOTLOADERPATH)\* $(PROJ_DIR)\components\bootloader\\
	copy /b $(PROJ_DIR)\components\bootloader\subproject\main\bootloader_start.c+,, $(PROJ_DIR)\components\bootloader\subproject\main\bootloader_start.c

$(XS_OBJ): $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

{$(XS_DIR)\platforms\esp\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

{$(XS_DIR)\platforms\mc\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

{$(PLATFORM_DIR)\lib\pow\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	@echo # cc $(@F) (slots in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	@echo # cc $(@F) (slots in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) <<args.txt
-b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)
<<

$(TMP_DIR)\mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) <<args.txt
$(DATA) $(RESOURCES) -o $(TMP_DIR) -p $(ESP32_SUBCLASS) -r mc.resources.c
<<
