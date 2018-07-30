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

HOST_OS = win

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

MSYS32_BASE = $(BASE_DIR)\msys32
!IF "$(IDF_PATH)"==""
IDF_PATH = $(MSYS32_BASE)\home\$(USERNAME)\esp\esp-idf
!ENDIF
!IF "$(TOOLS_ROOT)"==""
TOOLS_ROOT = $(MSYS32_BASE)\opt\xtensa-esp32-elf
!ENDIF

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp32\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp32\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\esp32\release\lib
!ENDIF

!IF "$(DEBUG)"=="1"
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\esp32\debug\idf
!ELSE
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\esp32\release\idf
!ENDIF

PLATFORM_DIR = $(BUILD_DIR)\devices\esp32
PROJ_DIR = $(PLATFORM_DIR)\xsProj

INC_DIRS = \
	-I$(IDF_PATH)\components \
	-I$(IDF_PATH)\components\heap\include \
	-I$(IDF_PATH)\components\driver\include \
	-I$(IDF_PATH)\components\soc\esp32\include \
	-I$(IDF_PATH)\components\soc\include \
	-I$(IDF_PATH)\components\esp32\include \
	-I$(IDF_PATH)/components\soc\esp32\include\soc \
	-I$(IDF_PATH)\components\freertos \
	-I$(IDF_PATH)\components\freertos\include \
	-I$(IDF_PATH)\components\freertos\include\freertos \
	-I$(IDF_PATH)\components\lwip\include\lwip \
	-I$(IDF_PATH)\components\lwip\include\lwip\port \
	-I$(IDF_PATH)\components\mbedtls\include \
	-I$(IDF_PATH)\components\spi_flash\include \
	-I$(IDF_PATH)\components\vfs\include \
	-I$(IDF_PATH)\components\tcpip_adapter\include \
	-I$(IDF_PATH)\components\tcpip_adapter \
	-I$(IDF_PATH)\components\bt\include \
	-I$(IDF_PATH)\components\bt\bluedroid\api\include \
	-I$(IDF_PATH)\components\bt\bluedroid\api\include\api \
	-I$(IDF_PATH)\components\newlib\include \
	-I$(IDF_PATH)\components\newlib\platform_include
	
XS_OBJ = \
	$(LIB_DIR)\xsHost.o \
	$(LIB_DIR)\xsPlatform.o \
	$(LIB_DIR)\xsAll.o \
	$(LIB_DIR)\xsAPI.o \
	$(LIB_DIR)\xsArguments.o \
	$(LIB_DIR)\xsArray.o \
	$(LIB_DIR)\xsAtomics.o \
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

XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\platforms\esp \
	-I$(IDF_BUILD_DIR)\include \
	-I$(PLATFORM_DIR)\lib\pow

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsesp.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\esp\xsPlatform.h

SDKCONFIG =\
	$(PROJ_DIR)\sdkconfig.default

HEADERS = $(HEADERS) $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)\bin
CC = $(TOOLS_BIN)\xtensa-esp32-elf-gcc
CPP = $(TOOLS_BIN)\xtensa-esp32-elf-g++
LD = $(CPP)
AR = $(TOOLS_BIN)\xtensa-esp32-elf-ar
OBJCOPY = $(TOOLS_BIN)\xtensa-esp32-elf-objcopy

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
!ELSE
LAUNCH = release
!ENDIF

.PHONY: all

all: $(BLE) $(SDKCONFIG) $(LAUNCH)

!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ENDIF

debug: $(LIB_DIR) $(BIN_DIR)\xs_esp.a
	-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
	tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp.a $(IDF_BUILD_DIR)\.
	set HOME=$(PROJ_DIR)
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make flash; echo Launching app...; echo -e '\nType Ctrl-C to close this window'; $(SERIAL2XSBUG) $(UPLOAD_PORT) 921600 8N1 | more"

release: $(LIB_DIR) $(BIN_DIR)\xs_esp.a
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp.a $(IDF_BUILD_DIR)\.
	set HOME=$(PROJ_DIR)
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=0 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make flash monitor;"

$(PROJ_DIR)\sdkconfig.default:
	if exist $(TMP_DIR)\_s.tmp del $(TMP_DIR)\_s.tmp
!IF !EXIST($(IDF_BUILD_DIR)\)
	echo 1 > $(TMP_DIR)\_s.tmp
!ENDIF
!IF !EXIST($(SDKCONFIG_FILE).prior)
	echo 1 > $(TMP_DIR)\_s.tmp
!ELSE
	-FC $(SDKCONFIG_FILE) $(SDKCONFIG_FILE).prior | (find "CONFIG_" > nul) && (echo 1 > $(TMP_DIR)\_s.tmp)
!ENDIF
	set HOME=$(PROJ_DIR)
	if exist $(TMP_DIR)\_s.tmp (if exist $(PROJ_DIR)\sdkconfig del $(PROJ_DIR)\sdkconfig)
	if exist $(TMP_DIR)\_s.tmp (copy $(SDKCONFIG_FILE) $(SDKCONFIG_FILE).prior)
	if exist $(TMP_DIR)\_s.tmp ($(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Running GENCONFIG...; BATCH_BUILD=1 DEBUG=$(DEBUG) IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make defconfig")
	if exist $(TMP_DIR)\_s.tmp (@echo.)
	if exist $(TMP_DIR)\_s.tmp (@echo Press any key to complete build **after** MinGW x32 console window closes...)
	if exist $(TMP_DIR)\_s.tmp (pause>nul)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h

$(BIN_DIR)\xs_esp.a: $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS)
	@echo # ld xs_esp.bin
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.c.o
	$(AR) $(AR_OPTIONS) $(BIN_DIR)\xs_esp.a $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(LIB_DIR)\buildinfo.c.o

$(XS_OBJ):$(XS_HEADERS)
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
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)

$(TMP_DIR)\mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp32 -r mc.resources.c
