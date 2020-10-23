#
# Copyright (c) 2016-2020  Moddable Tech, Inc.
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

!IF "$(EXPECTED_ESP_IDF)"==""
EXPECTED_ESP_IDF = v3.3.2
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

!IF "$(UPLOAD_SPEED)"==""
UPLOAD_SPEED = 921600
!ENDIF
!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 460800
!ENDIF

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

!IF "$(ESP32_CMAKE)"==""
ESP32_CMAKE = 1
!ENDIF

!IF "$(ESP32_CMAKE)"!="1"
!IF "$(UPLOAD_PORT)"==""
!ERROR UPLOAD_PORT environment variable must be defined!
!ENDIF
!ENDIF

!IF "$(ESP32_CMAKE)"!="1"
!IF "$(SERIAL2XSBUG)"==""
!IF "$(DEBUG)"=="1"
!ERROR SERIAL2XSBUG environment variable must be defined!
!ENDIF
!ENDIF
!ENDIF


MSYS32_BASE = $(BASE_DIR)\msys32
!IF "$(IDF_PATH)"==""
IDF_PATH = $(MSYS32_BASE)\home\$(USERNAME)\esp\esp-idf
!ENDIF
!IF "$(TOOLS_ROOT)"==""
TOOLS_ROOT = $(MSYS32_BASE)\opt\xtensa-esp32-elf
!ENDIF

!IF [cd $(IDF_PATH) && git describe --always > $(TMP_DIR)\_idf_version.tmp 2> nul] == 0
IDF_VERSION = \
!INCLUDE $(TMP_DIR)\_idf_version.tmp
!IF [del $(TMP_DIR)\_idf_version.tmp] == 0
!ENDIF
!IF "$(IDF_VERSION)"!="$(EXPECTED_ESP_IDF)"
!ERROR Detected ESP-IDF version $(IDF_VERSION). Expected ESP-IDF version $(EXPECTED_ESP_IDF).
!ENDIF
!ELSE
!MESSAGE Could not detect ESP-IDF version.
!ENDIF

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\lib
!ENDIF

PROJ_DIR_TEMPLATE = $(BUILD_DIR)\devices\esp32\xsProj
!IF "$(ESP32_CMAKE)"=="1"

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

!ENDIF

!IF "$(DEBUG)"=="1"
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\idf
PROJ_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\xsProj
KILL_SERIAL2XSBUG= -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
START_XSBUG= tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)

!IF "$(ESP32_CMAKE)"=="1"
BUILD_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) -B $(IDF_BUILD_DIR) build -D mxDebug=1 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
BUILD_MSG =
DEPLOY_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) -B $(IDF_BUILD_DIR) flash -D mxDebug=1 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
START_SERIAL2XSBUG = echo Launching app... & echo Type Ctrl-C twice after debugging app. & $(BUILD_DIR)\bin\win\release\serial2xsbug $(PORT_TO_USE) $(DEBUGGER_SPEED) 8N1
!ELSE
BUILD_CMD = $(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make ; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/bootloader/bootloader.bin $(BIN_DIR_MINGW)"
BUILD_MSG = echo Build is complete when the MinGW windows closes.
DEPLOY_CMD = $(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Deploying xs_esp32.elf...; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make flash"
START_SERIAL2XSBUG = $(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c " echo Launching app...; echo -e '\nType Ctrl-C after debugging app to close this window'; $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 | more"
!ENDIF

!ELSE
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\idf
PROJ_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\xsProj
KILL_SERIAL2XSBUG= -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
START_XSBUG=
START_SERIAL2XSBUG = echo No debugger for a release build.

!IF "$(ESP32_CMAKE)"=="1"
BUILD_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) -B $(IDF_BUILD_DIR) build -D mxDebug=0 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
DEPLOY_CMD = python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) -B $(IDF_BUILD_DIR) flash -D mxDebug=0 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
!ELSE
BUILD_CMD = $(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=0 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/bootloader/bootloader.bin $(BIN_DIR_MINGW)"
DEPLOY_CMD = $(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Deploying xs_esp32.elf...; DEBUG=0 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make flash"
!ENDIF

!ENDIF

PLATFORM_DIR = $(BUILD_DIR)\devices\esp32

INC_DIRS = \
 	-I$(IDF_PATH)\components \
 	-I$(IDF_PATH)\components\bootloader_support\include \
 	-I$(IDF_PATH)\components\bt\include \
 	-I$(IDF_PATH)\components\bt\bluedroid\api\include \
 	-I$(IDF_PATH)\components\bt\bluedroid\api\include\api \
 	-I$(IDF_PATH)\components\driver\include \
 	-I$(IDF_PATH)\components\esp32\include \
 	-I$(IDF_PATH)\components\esp_event\include \
 	-I$(IDF_PATH)\components\esp_ringbuf\include \
 	-I$(IDF_PATH)\components\freertos \
 	-I$(IDF_PATH)\components\freertos\include \
 	-I$(IDF_PATH)\components\freertos\include\freertos \
	-I$(IDF_PATH)\components\heap\include \
	-I$(IDF_PATH)\components\log\include \
	-I$(IDF_PATH)\components\lwip\include\apps \
	-I$(IDF_PATH)\components\lwip\include\apps\sntp \
	-I$(IDF_PATH)\components\lwip\lwip\src\include \
	-I$(IDF_PATH)\components\lwip\port\esp32\include \
	-I$(IDF_PATH)\components\mbedtls\include \
	-I$(IDF_PATH)\components\newlib\include \
	-I$(IDF_PATH)\components\newlib\platform_include \
	-I$(IDF_PATH)\components\nimble\esp-hci\include \
	-I$(IDF_PATH)\components\nimble\nimble\nimble\host\include \
	-I$(IDF_PATH)\components\nimble\nimble\nimble\host\services\gap\include \
	-I$(IDF_PATH)\components\nimble\nimble\nimble\host\src \
	-I$(IDF_PATH)\components\nimble\nimble\nimble\include \
	-I$(IDF_PATH)\components\nimble\nimble\nimble\include\nimble \
	-I$(IDF_PATH)\components\nimble\nimble\porting\nimble\include \
	-I$(IDF_PATH)\components\nimble\nimble\porting\npl\freertos\include \
	-I$(IDF_PATH)\components\nimble\port\include \
	-I$(IDF_PATH)\components\soc\esp32\include \
	-I$(IDF_PATH)\components\soc\esp32\include\soc \
	-I$(IDF_PATH)\components\soc\include \
	-I$(IDF_PATH)\components\spiffs\include \
	-I$(IDF_PATH)\components\fatfs\src \
	-I$(IDF_PATH)\components\wear_levelling\include \
	-I$(IDF_PATH)\components\spi_flash\include \
	-I$(IDF_PATH)\components\tcpip_adapter\include \
	-I$(IDF_PATH)\components\tcpip_adapter \
	-I$(IDF_PATH)\components\vfs\include

XS_OBJ = \
	$(LIB_DIR)\xsHost.o \
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

!IF "$(ESP32_CMAKE)"=="1"
SDKCONFIG_H_DIR = $(IDF_BUILD_DIR)\config
!ELSE
SDKCONFIG_H_DIR = $(IDF_BUILD_DIR)\include
!ENDIF

XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\platforms\esp \
	-I$(SDKCONFIG_H_DIR) \
	-I$(PLATFORM_DIR)\lib\pow

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\esp\xsHost.h \
	$(XS_DIR)\platforms\esp\xsPlatform.h

!IF "$(SDKCONFIGPATH)"==""
SDKCONFIGPATH = $(PROJ_DIR)
!ENDIF
SDKCONFIG = $(SDKCONFIGPATH)\sdkconfig.defaults
SDKCONFIGPRIOR = $(SDKCONFIGPATH)\sdkconfig.defaults.prior
SDKCONFIG_H = $(SDKCONFIG_H_DIR)\sdkconfig.h

HEADERS = $(HEADERS) $(XS_HEADERS)

!IF "$(ESP32_CMAKE)"=="1"
TOOLS_BIN = 
!ELSE
TOOLS_BIN = $(TOOLS_ROOT)\bin\ 
!ENDIF

CC = $(TOOLS_BIN)xtensa-esp32-elf-gcc
CPP = $(TOOLS_BIN)xtensa-esp32-elf-g++
LD = $(CPP)
AR = $(TOOLS_BIN)xtensa-esp32-elf-ar
OBJCOPY = $(TOOLS_BIN)xtensa-esp32-elf-objcopy

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
	-DESP32=1 \
	$(NET_CONFIG_FLAGS) \
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

C_FLAGS = $(C_COMMON_FLAGS) \
	-Wno-implicit-function-declaration \
	-std=gnu99

CPP_FLAGS = $(C_COMMON_FLAGS)

!IF "$(DEBUG)"=="1"
LAUNCH = debug
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ELSE
LAUNCH = release
!ENDIF

!IF "$(PARTITIONS_FILE)"==""
PARTITIONS_FILE = $(PROJ_DIR_TEMPLATE)\partitions.csv
!ENDIF

PARTITIONS_BIN = partition-table.bin
PARTITIONS_PATH = $(IDF_BUILD_DIR)\partition_table\$(PARTITIONS_BIN)

!IF [fc $(PARTITIONS_FILE) $(PROJ_DIR)\partitions.csv > nul] == 1
!IF [copy /Y $(PARTITIONS_FILE) $(PROJ_DIR)\partitions.csv] == 0
!IF [copy /b $(PROJ_DIR)\partitions.csv+,, $(PROJ_DIR)\partitions.csv] == 0
!ENDIF
!ENDIF
!ENDIF

PROJ_DIR_FILES = \
	$(PROJ_DIR)\main\main.c	\
	$(PROJ_DIR)\main\component.mk	\
	$(PROJ_DIR)\partitions.csv \
	$(PROJ_DIR)\Makefile

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
	echo $(IDF_BUILD_DIR)
	if exist $(IDF_BUILD_DIR) del /s/q/f $(IDF_BUILD_DIR)\*.* > NUL
	if exist $(IDF_BUILD_DIR) rmdir /s/q $(IDF_BUILD_DIR)
	echo $(PROJ_DIR)
	if exist $(PROJ_DIR) del /s/q/f $(PROJ_DIR)\*.* > NUL
	if exist $(PROJ_DIR) rmdir /s/q $(PROJ_DIR)

precursor: projDir $(BLE) $(SDKCONFIG_H) $(LIB_DIR) $(BIN_DIR)\xs_esp32.a


debug: precursor
	-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
	tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp32.a $(IDF_BUILD_DIR)\.
!IF "$(ESP32_CMAKE)"=="1"
	cd $(PROJ_DIR)
	python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) -B $(IDF_BUILD_DIR) build flash -D mxDebug=1 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
	copy $(IDF_BUILD_DIR)\xs_esp32.map $(BIN_DIR)\.
	copy $(IDF_BUILD_DIR)\xs_esp32.bin $(BIN_DIR)\.
	copy $(IDF_BUILD_DIR)\partition_table\partition-table.bin $(BIN_DIR)
	copy $(IDF_BUILD_DIR)\bootloader\bootloader.bin $(BIN_DIR)\.
	(@echo Launching app. Type Ctrl-C twice after debugging app to close serial2xsbug...)
	$(BUILD_DIR)\bin\win\release\serial2xsbug $(PORT_TO_USE) $(DEBUGGER_SPEED) 8N1
!ELSE
	set HOME=$(PROJ_DIR)
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make flash; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/bootloader/bootloader.bin $(BIN_DIR_MINGW); echo Launching app...; echo -e '\nType Ctrl-C after debugging app to close this window'; $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 | more"
!ENDIF

release: precursor
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp32.a $(IDF_BUILD_DIR)\.
!IF "$(ESP32_CMAKE)"=="1"
	cd $(PROJ_DIR)
	python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) -B $(IDF_BUILD_DIR) build flash -D mxDebug=0 SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
	copy $(IDF_BUILD_DIR)\xs_esp32.map $(BIN_DIR)\.
	copy $(IDF_BUILD_DIR)\xs_esp32.bin $(BIN_DIR)\.
	copy $(IDF_BUILD_DIR)\partition_table\partition-table.bin $(BIN_DIR)
	copy $(IDF_BUILD_DIR)\bootloader\bootloader.bin $(BIN_DIR)\.
	python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) $(PORT_COMMAND) -b $(UPLOAD_SPEED) -B $(IDF_BUILD_DIR) monitor -D CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL)
!ELSE
	set HOME=$(PROJ_DIR)
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=0 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make flash; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/bootloader/bootloader.bin $(BIN_DIR_MINGW); make monitor;"
!ENDIF


mingPrepare:
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp32.a $(IDF_BUILD_DIR)\.
	set HOME=$(PROJ_DIR)
	cd $(PROJ_DIR)
		
build: precursor mingPrepare
	echo $(BUILD_CMD)
	$(BUILD_CMD)
	$(BUILD_MSG)
	copy $(IDF_BUILD_DIR)\bootloader\bootloader.bin $(BIN_DIR)
	copy $(IDF_BUILD_DIR)\partition_table\partition-table.bin $(BIN_DIR)
	if exist $(IDF_BUILD_DIR)\ota_data_initial.bin copy $(IDF_BUILD_DIR)\ota_data_initial.bin $(BIN_DIR)
	copy $(IDF_BUILD_DIR)\xs_esp32.bin $(BIN_DIR)
	copy $(IDF_BUILD_DIR)\xs_esp32.map $(BIN_DIR)

xsbug:
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(START_SERIAL2XSBUG)

DEPLOY_PRE:
	$(KILL_SERIAL2XSBUG)
	if not exist $(BIN_DIR)\xs_esp32.bin echo "Please build before deploy"
	if not exist $(BIN_DIR)\xs_esp32.bin exit 1
	if exist $(IDF_BUILD_DIR)\xs_esp32.bin move /Y $(IDF_BUILD_DIR)\xs_esp32.bin $(IDF_BUILD_DIR)\xs_esp32.bin_prev
	if exist $(PARTITIONS_PATH) move /Y $(PARTITIONS_PATH) $(PARTITIONS_PATH)_prev
	if exist $(IDF_BUILD_DIR)\bootloader\bootloader.bin move /Y $(IDF_BUILD_DIR)\bootloader\bootloader.bin $(IDF_BUILD_DIR)\bootloader\bootloader.bin_prev
	if exist $(IDF_BUILD_DIR)\ota_data_initial.bin move /Y $(IDF_BUILD_DIR)\ota_data_initial.bin $(IDF_BUILD_DIR)\ota_data_initial.bin_prev

DEPLOY_START:
	if exist $(BIN_DIR)\xs_esp32.bin copy $(BIN_DIR)\xs_esp32.bin $(IDF_BUILD_DIR)
	if exist $(BIN_DIR)\$(PARTITIONS_BIN) copy $(BIN_DIR)\$(PARTITIONS_BIN) $(PARTITIONS_PATH)
	if exist $(BIN_DIR)\bootloader.bin  copy $(BIN_DIR)\bootloader.bin $(IDF_BUILD_DIR)\bootloader\bootloader.bin
	if exist $(BIN_DIR)\ota_data_initial.bin copy $(BIN_DIR)\ota_data_initial.bin $(IDF_BUILD_DIR)\ota_data_initial.bin
	set HOME=$(PROJ_DIR)
	cd $(PROJ_DIR)
	echo $(DEPLOY_CMD)
	$(DEPLOY_CMD)

DEPLOY_END:
	if exist $(IDF_BUILD_DIR)\xs_esp32.bin del $(IDF_BUILD_DIR)\xs_esp32.bin
	if exist $(PARTITIONS_PATH) del $(PARTITIONS_PATH)
	if exist $(IDF_BUILD_DIR)\bootloader\bootloader.bin del $(IDF_BUILD_DIR)\bootloader\bootloader.bin
	if exist $(IDF_BUILD_DIR)\ota_data_initial.bin del $(IDF_BUILD_DIR)\ota_data_initial.bin
	if exist $(IDF_BUILD_DIR)\xs_esp32.bin_prev move /Y $(IDF_BUILD_DIR)\xs_esp32.bin_prev $(IDF_BUILD_DIR)\xs_esp32.bin
	if exist $(PARTITIONS_PATH)_prev move /Y $(PARTITIONS_PATH)_prev $(PARTITIONS_PATH)
	if exist $(IDF_BUILD_DIR)\bootloader\bootloader.bin_prev move /Y $(IDF_BUILD_DIR)\bootloader\bootloader.bin_prev $(IDF_BUILD_DIR)\bootloader\bootloader.bin
	if exist $(IDF_BUILD_DIR)\ota_data_initial.bin_prev move /Y $(IDF_BUILD_DIR)\ota_data_initial.bin_prev $(IDF_BUILD_DIR)\ota_data_initial.bin

deploy: DEPLOY_PRE DEPLOY_START DEPLOY_END

$(SDKCONFIG_H): $(SDKCONFIG_FILE)
	if exist $(TMP_DIR)\_s.tmp del $(TMP_DIR)\_s.tmp
	if exist $(TMP_DIR)\_fc.tmp del $(TMP_DIR)\_fc.tmp
!IF !EXIST($(SDKCONFIGPRIOR))
	copy $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR)
!ENDIF
!IF !EXIST($(IDF_BUILD_DIR)\)
	if exist $(SDKCONFIGPRIOR) del $(SDKCONFIGPRIOR)
	echo 1 > $(TMP_DIR)\_s.tmp
!ENDIF
	if exist $(SDKCONFIG_H) ( echo 1 > nul ) else ( echo 1 > $(TMP_DIR)\_s.tmp )
	-if exist $(SDKCONFIGPRIOR) (fc $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR) > $(TMP_DIR)\_fc.tmp)
	-if exist $(TMP_DIR)\_fc.tmp ((find "CONFIG_" $(TMP_DIR)\_fc.tmp > nul) && echo 1 > $(TMP_DIR)\_s.tmp )
	if exist $(TMP_DIR)\_s.tmp (if exist $(PROJ_DIR)\sdkconfig del $(PROJ_DIR)\sdkconfig)
	if exist $(TMP_DIR)\_s.tmp (copy $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR))
	@echo Reconfiguring ESP-IDF...
!IF "$(ESP32_CMAKE)"=="1"
	cd $(PROJ_DIR) 
	python %IDF_PATH%\tools\idf.py $(IDF_PY_LOG_FLAG) -B $(IDF_BUILD_DIR) reconfigure -D SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) SDKCONFIG_H="$(SDKCONFIG_H)" CMAKE_MESSAGE_LOG_LEVEL=$(CMAKE_LOG_LEVEL) DEBUGGER_SPEED=$(DEBUGGER_SPEED)
	COPY /B $(SDKCONFIG_H)+,, $(SDKCONFIG_H)
!ELSE
	set HOME=$(PROJ_DIR)
	if exist $(TMP_DIR)\_s.tmp ($(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "BATCH_BUILD=1 DEBUG=$(DEBUG) IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make defconfig")
	if exist $(TMP_DIR)\_s.tmp (@echo.)
	if exist $(TMP_DIR)\_s.tmp (@echo Press any key to complete build **after** MinGW x32 console window closes...)
	if exist $(TMP_DIR)\_s.tmp (pause>nul)
!ENDIF
	if exist $(PROJ_DIR)\sdkconfig.old (copy $(PROJ_DIR)\sdkconfig.old $(SDKCONFIGPATH)\sdkconfig.old)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h

$(BIN_DIR)\xs_esp32.a: $(PROJ_DIR)\main\main.c $(SDKCONFIG_H) $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS)
	@echo # ld xs_esp32.bin
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.c.o
	$(AR) $(AR_OPTIONS) $(BIN_DIR)\xs_esp32.a $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(LIB_DIR)\buildinfo.c.o

projDir: $(PROJ_DIR) $(PROJ_DIR_FILES) $(PROJ_DIR)\partitions.csv

$(PROJ_DIR) : $(PROJ_DIR_TEMPLATE)
	echo d | xcopy /s $(PROJ_DIR_TEMPLATE) $(PROJ_DIR)
	copy $(PARTITIONS_FILE) $(PROJ_DIR)\partitions.csv

$(PROJ_DIR)\partitions.csv: $(PARTITIONS_FILE)
	copy $? $@

$(PROJ_DIR)\main\main.c: $(PROJ_DIR_TEMPLATE)\main\main.c
	copy $? $@

$(PROJ_DIR)\main\component.mk: $(PROJ_DIR_TEMPLATE)\main\component.mk
	copy $? $@

$(PROJ_DIR)\Makefile: $(PROJ_DIR_TEMPLATE)\Makefile
	copy $? $@

$(XS_OBJ): $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

{$(XS_DIR)\platforms\esp\}.c{$(LIB_DIR)\}.o:
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
$(DATA) $(RESOURCES) -o $(TMP_DIR) -p esp32 -r mc.resources.c
<<
