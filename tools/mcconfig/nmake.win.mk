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

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\win\debug\mc\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\win\instrument\mc\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\win\release\mc\lib
!ENDIF

XS_DIRECTORIES = \
	/I$(XS_DIR)\includes \
	/I$(XS_DIR)\platforms \
	/I$(XS_DIR)\sources

XS_HEADERS = \
	$(XS_DIR)\platforms\win_xs.h \
	$(XS_DIR)\platforms\xsPlatform.h \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)\win_xs.o \
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
	$(LIB_DIR)\xsPlatforms.o \
	$(LIB_DIR)\xsProfile.o \
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

DIRECTORIES = $(DIRECTORIES) $(XS_DIRECTORIES)
	
HEADERS = $(HEADERS) $(XS_HEADERS)

C_DEFINES = \
	/D XS_ARCHIVE=1 \
	/D INCLUDE_XSPLATFORM=1 \
	/D XSPLATFORM=\"win_xs.h\" \
	/D mxRun=1 \
	/D mxNoFunctionLength=1 \
	/D mxNoFunctionName=1 \
	/D mxHostFunctionPrimitive=1 \
	/D mxFewGlobalsTable=1 \
	/D kCommodettoBitmapFormat=$(DISPLAY) \
	/D kPocoRotation=$(ROTATION)
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) \
	/D MODINSTRUMENTATION=1 \
	/D mxInstrument=1
!ENDIF

C_INCLUDES = $(DIRECTORIES) /I$(TMP_DIR)

C_FLAGS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/nologo \
	/Zp1 
!IF "$(DEBUG)"=="1"
C_FLAGS = $(C_FLAGS) \
	/D _DEBUG \
	/D mxDebug=1 \
	/Fp$(TMP_DIR)\ \
	/Od \
	/W3 \
	/Z7
!ELSE
C_FLAGS = $(C_FLAGS) \
	/D NDEBUG \
	/Fp$(TMP_DIR)\ \
	/O2 \
	/W0
!ENDIF

LINK_LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib dsound.lib wlanapi.lib Iphlpapi.lib

LINK_OPTIONS = /incremental:no /machine:I386 /nologo /dll
!IF "$(DEBUG)"=="1"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

BUILDCLUT = $(BUILD_DIR)\bin\win\debug\buildclut
COMPRESSBMF = $(BUILD_DIR)\bin\win\debug\compressbmf
IMAGE2CS = $(BUILD_DIR)\bin\win\debug\image2cs
MCLOCAL = $(BUILD_DIR)\bin\win\debug\mclocal
MCREZ = $(BUILD_DIR)\bin\win\debug\mcrez
PNG2BMP = $(BUILD_DIR)\bin\win\debug\png2bmp
RLE4ENCODE = $(BUILD_DIR)\bin\win\debug\rle4encode
WAV2MAUD = $(BUILD_DIR)\bin\win\debug\wav2maud
XSC = $(BUILD_DIR)\bin\win\debug\xsc
XSID = $(BUILD_DIR)\bin\win\debug\xsid
XSL = $(BUILD_DIR)\bin\win\debug\xsl
	
all: $(LIB_DIR) $(BIN_DIR)\mc.dll $(DATA)
	start $(SIMULATOR) $(BIN_DIR)\mc.dll

$(LIB_DIR) :
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

$(BIN_DIR)\mc.dll: $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) 
	@echo # link mc.dll
	link $(LINK_OPTIONS) $(LINK_LIBRARIES) $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) /implib:$(TMP_DIR)\mc.lib /out:$@
	
$(XS_OBJECTS) : $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	cl $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< /Fo$@
{$(XS_DIR)\platforms\}.c{$(LIB_DIR)\}.o:
	cl $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< /Fo$@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c $(HEADERS)
	cl $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(TMP_DIR)\mc.xs.c /Fo$@
	
$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) <<args.txt 
-b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)
<<

$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c $(HEADERS)
	cl $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $(TMP_DIR)\mc.resources.c /Fo$@

$(TMP_DIR)\mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) <<args.txt
$(RESOURCES) -o $(TMP_DIR) -r mc.resources.c
<<
