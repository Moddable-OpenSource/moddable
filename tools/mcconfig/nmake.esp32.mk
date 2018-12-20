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

!IF "$(UPLOAD_SPEED)"==""
UPLOAD_SPEED = 921600
!ENDIF
!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 460800
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
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\lib
!ENDIF

!IF "$(DEBUG)"=="1"
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\idf
PROJ_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\xsProj
!ELSE
IDF_BUILD_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\idf
PROJ_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\xsProj
!ENDIF

PLATFORM_DIR = $(BUILD_DIR)\devices\esp32

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
	-I$(IDF_PATH)\components\lwip\lwip\src\include \
	-I$(IDF_PATH)\components\lwip\port\esp32\include \
	-I$(IDF_PATH)\components\lwip\include\apps \
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

!IF "$(SDKCONFIGPATH)"==""
SDKCONFIGPATH = $(PROJ_DIR)
!ENDIF
SDKCONFIG = $(SDKCONFIGPATH)\sdkconfig.defaults
SDKCONFIGPRIOR = $(SDKCONFIGPATH)\sdkconfig.defaults.prior
SDKCONFIG_H = $(IDF_BUILD_DIR)\include\sdkconfig.h

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
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ELSE
LAUNCH = release
!ENDIF

PROJ_DIR_TEMPLATE = $(BUILD_DIR)\devices\esp32\xsProj

!IF "$(PARTITIONS_FILE)"==""
PARTITIONS_FILE = $(PROJ_DIR_TEMPLATE)\partitions.csv
!ENDIF

PROJ_DIR_FILES = \
	$(PROJ_DIR)\main\main.c	\
	$(PROJ_DIR)\main\component.mk	\
	$(PROJ_DIR)\partitions.csv \
	$(PROJ_DIR)\Makefile

.PHONY: all

all: projDir $(BLE) $(SDKCONFIG_H) $(LAUNCH)

debug: $(LIB_DIR) $(BIN_DIR)\xs_esp32.a
	-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
	tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp32.a $(IDF_BUILD_DIR)\.
	set HOME=$(PROJ_DIR)
	@echo "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make flash; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); echo Launching app...; echo -e '\nType Ctrl-C to close this window'; $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 | more"
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=1 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) DEBUGGER_SPEED=$(DEBUGGER_SPEED) make flash; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); echo Launching app...; echo -e '\nType Ctrl-C to close this window'; $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 | more"

release: $(LIB_DIR) $(BIN_DIR)\xs_esp32.a
	if exist $(IDF_BUILD_DIR)\xs_esp32.elf del $(IDF_BUILD_DIR)\xs_esp32.elf
	if not exist $(IDF_BUILD_DIR) mkdir $(IDF_BUILD_DIR)
	copy $(BIN_DIR)\xs_esp32.a $(IDF_BUILD_DIR)\.
	set HOME=$(PROJ_DIR)
	$(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Building xs_esp32.elf...; touch ./main/main.c; DEBUG=0 IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make flash; cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.map $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/xs_esp32.bin $(BIN_DIR_MINGW); cp $(IDF_BUILD_DIR_MINGW)/partitions.bin $(BIN_DIR_MINGW); make monitor;"

$(SDKCONFIG_H): $(SDKCONFIG_FILE)
	if exist $(TMP_DIR)\_s.tmp del $(TMP_DIR)\_s.tmp
!IF !EXIST($(SDKCONFIGPRIOR))
	copy $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR)
	@echo "# no .prior - try current"
!ENDIF
!if !EXIST($(IDF_BUILD_DIR)\)
	if exist $(SDKCONFIGPRIOR) del $(SDKCONFIGPRIOR)
	@echo "# no idf_build_dir - remove .prior"
	echo 1 > $(TMP_DIR)\_s.tmp
!ENDIF
!if !EXIST($(SDKCONFIG_H)\)
	@echo "# no sdkconfig.h - generate it"
	echo 1 > $(TMP_DIR)\_s.tmp
!endif
	-FC $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR) | (find "CONFIG_" > nul) && (echo 1 > $(TMP_DIR)\_s.tmp)
	set HOME=$(PROJ_DIR)
	if exist $(TMP_DIR)\_s.tmp (if exist $(PROJ_DIR)\sdkconfig del $(PROJ_DIR)\sdkconfig)
	if exist $(TMP_DIR)\_s.tmp (copy $(SDKCONFIG_FILE) $(SDKCONFIGPRIOR))
	if exist $(TMP_DIR)\_s.tmp ($(MSYS32_BASE)\msys2_shell.cmd -mingw32 -c "echo Running GENCONFIG...; BATCH_BUILD=1 DEBUG=$(DEBUG) IDF_BUILD_DIR=$(IDF_BUILD_DIR_MINGW) SDKCONFIG_DEFAULTS=$(SDKCONFIG_FILE_MINGW) make defconfig")
	if exist $(PROJ_DIR)\sdkconfig.old (copy $(PROJ_DIR)\sdkconfig.old $(SDKCONFIGPATH)\sdkconfig.old)
	if exist $(TMP_DIR)\_s.tmp (@echo.)
	if exist $(TMP_DIR)\_s.tmp (@echo Press any key to complete build **after** MinGW x32 console window closes...)
	if exist $(TMP_DIR)\_s.tmp (pause>nul)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h

$(BIN_DIR)\xs_esp32.a: $(PROJ_DIR)\main\main.c $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS)
	@echo # ld xs_esp32.bin
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.c.o
	$(AR) $(AR_OPTIONS) $(BIN_DIR)\xs_esp32.a $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(LIB_DIR)\buildinfo.c.o

projDir: $(PROJ_DIR) $(PROJ_DIR_FILES) $(PARTITIONS_FILE)

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

$(XS_OBJ): $(SDKCONFIG_H) $(XS_HEADERS)
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
