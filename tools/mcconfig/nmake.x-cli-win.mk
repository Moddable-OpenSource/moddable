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
LIB_DIR = $(BUILD_DIR)\tmp\win\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\win\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\win\release\lib
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
	/D mxParse=1 \
	/D mxNoFunctionLength=1 \
	/D mxNoFunctionName=1 \
	/D mxHostFunctionPrimitive=1 \
	/D mxFewGlobalsTable=1
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) \
	/D MODINSTRUMENTATION=1 \
	/D mxInstrument=1
!ENDIF

C_INCLUDES = $(DIRECTORIES) /I$(TMP_DIR)

XS_C_FLAGS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/nologo \
	/Zp1 
!IF "$(DEBUG)"=="1"
XS_C_FLAGS = $(XS_C_FLAGS) \
	/D _DEBUG \
	/D mxDebug=1 \
	/Fp$(TMP_DIR)\ \
	/Od \
	/W3 \
	/Z7
!ELSE
XS_C_FLAGS = $(XS_C_FLAGS) \
	/D NDEBUG \
	/Fp$(TMP_DIR)\ \
	/O2 \
	/W0
!ENDIF
C_FLAGS = $(XS_C_FLAGS)

RC_OPTIONS = /nologo

LINK_LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib Iphlpapi.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo /subsystem:console
!IF "$(DEBUG)"=="1"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

MCLOCAL = $(BUILD_DIR)/bin/mac/debug/mclocal
MCREZ = $(BUILD_DIR)\bin\win\debug\mcrez
XSC = $(BUILD_DIR)\bin\win\debug\xsc
XSID = $(BUILD_DIR)\bin\win\debug\xsid
XSL = $(BUILD_DIR)\bin\win\debug\xsl
	
all: $(LIB_DIR) $(BIN_DIR)\$(NAME).exe 

$(LIB_DIR) :
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

$(BIN_DIR)\$(NAME).exe: $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(OBJECTS)
	@echo # link $(NAME).exe
	link $(LINK_OPTIONS) $(LINK_LIBRARIES) $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(OBJECTS) /implib:$(TMP_DIR)\$(NAME).lib /out:$@
	
$(XS_OBJECTS) : $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	cl $(C_DEFINES) $(C_INCLUDES) $(XS_C_FLAGS) $< /Fo$@
{$(XS_DIR)\platforms\}.c{$(LIB_DIR)\}.o:
	cl $(C_DEFINES) $(C_INCLUDES) $(XS_C_FLAGS) $< /Fo$@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c $(HEADERS)
	cl $(C_DEFINES) $(C_INCLUDES) $(XS_C_FLAGS) $(TMP_DIR)\mc.xs.c /Fo$@
	
$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(CREATION) $(MODULES)
	
	
