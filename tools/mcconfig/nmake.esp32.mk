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

BASE = $(MODDABLE)\libraries\esp
HOST_OS = win

!IF "$(BASE_DIR)"==""
BASE_DIR = $(USERPROFILE)
!ENDIF

ESP32_BASE = $(BASE_DIR)\esp32
IDF_PATH = $(ESP32_BASE)\esp-idf
# export IDF_PATH
TOOLS_ROOT = $(ESP32_BASE)\xtensa-esp32-elf

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp32\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\esp32\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\esp32\release\lib
!ENDIF

INC_DIRS = \
	-I$(BASE) \
 	-I$(IDF_PATH)\components \
	-I$(IDF_PATH)\components\heap\include \
 	-I$(IDF_PATH)\components\driver\include \
 	-I$(IDF_PATH)\components\soc\esp32\include \
 	-I$(IDF_PATH)\components\soc\include \
 	-I$(IDF_PATH)\components\esp32\include \
 	-I$(IDF_PATH)\components\freertos \
 	-I$(IDF_PATH)\components\freertos\include \
 	-I$(IDF_PATH)\components\lwip\include\lwip \
 	-I$(IDF_PATH)\components\lwip\include\lwip\port \
 	-I$(IDF_PATH)\components\mbedtls\include \
 	-I$(IDF_PATH)\components\spi_flash\include \
 	-I$(IDF_PATH)\components\vfs\include \
 	-I$(IDF_PATH)\components\tcpip_adapter\include \
 	-I$(IDF_PATH)\components\tcpip_adapter
    
XS_OBJ = \
	$(LIB_DIR)\xsHost.c.o \
	$(LIB_DIR)\xsPlatform.c.o \
	$(LIB_DIR)\xsAll.c.o \
	$(LIB_DIR)\xsAPI.c.o \
	$(LIB_DIR)\xsArray.c.o \
	$(LIB_DIR)\xsAtomics.c.o \
	$(LIB_DIR)\xsBoolean.c.o \
	$(LIB_DIR)\xsCode.c.o \
	$(LIB_DIR)\xsCommon.c.o \
	$(LIB_DIR)\xsDataView.c.o \
	$(LIB_DIR)\xsDate.c.o \
	$(LIB_DIR)\xsDebug.c.o \
	$(LIB_DIR)\xsError.c.o \
	$(LIB_DIR)\xsFunction.c.o \
	$(LIB_DIR)\xsGenerator.c.o \
	$(LIB_DIR)\xsGlobal.c.o \
	$(LIB_DIR)\xsJSON.c.o \
	$(LIB_DIR)\xsLexical.c.o \
	$(LIB_DIR)\xsMapSet.c.o \
	$(LIB_DIR)\xsMarshall.c.o \
	$(LIB_DIR)\xsMath.c.o \
	$(LIB_DIR)\xsMemory.c.o \
	$(LIB_DIR)\xsModule.c.o \
	$(LIB_DIR)\xsNumber.c.o \
	$(LIB_DIR)\xsObject.c.o \
	$(LIB_DIR)\xsPromise.c.o \
	$(LIB_DIR)\xsProperty.c.o \
	$(LIB_DIR)\xsProxy.c.o \
	$(LIB_DIR)\xsRegExp.c.o \
	$(LIB_DIR)\xsRun.c.o \
	$(LIB_DIR)\xsScope.c.o \
	$(LIB_DIR)\xsScript.c.o \
	$(LIB_DIR)\xsSourceMap.c.o \
	$(LIB_DIR)\xsString.c.o \
	$(LIB_DIR)\xsSymbol.c.o \
	$(LIB_DIR)\xsSyntaxical.c.o \
	$(LIB_DIR)\xsTree.c.o \
	$(LIB_DIR)\xsType.c.o \
	$(LIB_DIR)\xsdtoa.c.o \
	$(LIB_DIR)\xsre.c.o \
	$(LIB_DIR)\xsmc.c.o

XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\sources\pcre \
	-I$(XS_DIR)\platforms\esp \
	-I$(BUILD_DIR)\devices\esp32

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsesp.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\esp\xsPlatform.h

HEADERS = $(HEADERS) $(XS_HEADERS)

TOOLS_BIN = $(TOOLS_ROOT)\bin
CC  = $(TOOLS_BIN)\xtensa-esp32-elf-gcc
CPP = $(TOOLS_BIN)\xtensa-esp32-elf-g++
LD  = $(CPP)
AR  = $(TOOLS_BIN)\xtensa-esp32-elf-ar
OBJCOPY = $(TOOLS_BIN)\xtensa-esp32-elf-objcopy
ESPTOOL = $(IDF_PATH)\components\esptool_py\esptool\esptool.py

AR_FLAGS = crs

MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\debug
BUILDCLUT = $(MODDABLE_TOOLS_DIR)\buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)\compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)\rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)\mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)\mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)\png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)\image2cs
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

# Utility functions
# git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
# SRC_GIT_VERSION = $(call git_description,$(BASE)/sources)
# ESP_GIT_VERSION = $(call git_description,$(ARDUINO_ROOT))
# time_string = $(shell perl -e 'use POSIX qw(strftime); print strftime($(1), localtime());')
# BUILD_DATE = $(call time_string,"%Y-%m-%d")
# BUILD_TIME = $(call time_string,"%H:%M:%S")
# MEM_USAGE = \
#  'while (<>) { \
#      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
#		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
#	 }\
#	 print "\# Memory usage\n";\
#	 print sprintf("\#  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

# VPATH += $(SDK_DIRS) $(XS_DIRS)

.PHONY: all	

PROJ_DIR = $(BUILD_DIR)\devices\esp32\xsProj

!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1
!ENDIF

!IF "$(DEBUG)"=="1"
KILL_SERIAL_2_XSBUG = -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
DO_XSBUG = tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(BUILD_DIR)\bin\win\release\xsbug.exe)
DO_LAUNCH = $(BUILD_DIR)\bin\win\release\serial2xsbug $(UPLOAD_PORT) 230400 8N1
!ELSE
KILL_SERIAL_2_XSBUG = 
DO_XSBUG = 
DO_LAUNCH = cd $(PROJ_DIR); nmake monitor
!ENDIF
	
all: $(LIB_DIR) $(BIN_DIR)/xs_esp.a
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	-rm $(PROJ_DIR)/build/xs_esp32.elf
	-mkdir $(PROJ_DIR)/build
	cp $(BIN_DIR)/xs_esp.a $(PROJ_DIR)/build/.
	touch $(PROJ_DIR)/main/main.c
	cd $(PROJ_DIR) ; DEBUG=$(DEBUG) make flash
	$(DO_LAUNCH)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h
	
$(BIN_DIR)\xs_esp.a: $(SDK_OBJ) $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) 
	@echo "# ld xs_esp.bin"
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(LIB_DIR)\buildinfo.c -o $(LIB_DIR)\buildinfo.o
	$(AR) $(AR_FLAGS) $(BIN_DIR)\xs_esp.a $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(LIB_DIR)\buildinfo.o

$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)\xs%.c.o: xs%.c
	@echo # cc $(@F) (strings in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(LIB_DIR)\%.S.o: %.S
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $? -o $@

$(LIB_DIR)\%.o: %.c
	@echo # cc $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(LIB_DIR)\%.cpp.o: %.cpp
	@echo # cpp $(@F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $? -o $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	@echo # cc $(@F) (slots in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	
$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	@echo # cc $(@F) (slots in flash)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)\mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p esp32 -r mc.resources.c


