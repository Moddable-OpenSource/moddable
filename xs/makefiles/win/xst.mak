#
# Copyright (c) 2016-2023 Moddable Tech, Inc.
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

!IFNDEF GOAL
GOAL = debug
!ENDIF
NAME = xst

!IFNDEF XS_DIR
XS_DIR = ..\..
!ENDIF

!IFNDEF BUILD_DIR
BUILD_DIR = ..\..\..\build
!ENDIF

BIN_DIR = $(BUILD_DIR)\bin\win\$(GOAL)
INC_DIR = $(XS_DIR)\includes
PLT_DIR = $(XS_DIR)\platforms
SRC_DIR = $(XS_DIR)\sources
TLS_DIR = $(XS_DIR)\tools
TMP_DIR = $(BUILD_DIR)\tmp\win\$(GOAL)\$(NAME)

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D YAML_DECLARE_STATIC \
	/D INCLUDE_XSPLATFORM \
	/D XSPLATFORM=\"xst.h\" \
	/D mxDebug=1 \
	/D mxDeepQual=1 \
	/D mxLockdown=1 \
	/D mxNoConsole=1 \
	/D mxParse=1 \
	/D mxProfile=1 \
	/D mxRun=1 \
	/D mxSloppy=1 \
	/D mxSnapshot=1 \
	/D mxRegExpUnicodePropertyEscapes=1 \
	/D mxStringNormalize=1 \
	/D mxMinusZero=1 \
	/I$(INC_DIR) \
	/I$(PLT_DIR) \
	/I$(SRC_DIR) \
	/I$(TLS_DIR) \
	/I$(TLS_DIR)\yaml \
	/nologo \
	/MP
!IF "$(GOAL)"=="debug"
C_OPTIONS = $(C_OPTIONS) \
	/D _DEBUG \
	/Fp$(TMP_DIR_DBG)\ \
	/Od \
	/W3 \
	/Z7
!ELSE
C_OPTIONS = $(C_OPTIONS) \
	/D mxMultipleThreads=1 \
	/D NDEBUG \
	/Fp$(TMP_DIR_RLS)\ \
	/O2 \
	/W0
!ENDIF

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib
	
LINK_OPTIONS = /incremental:no /nologo /subsystem:console
!IF "$(GOAL)"=="debug"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

OBJECTS = \
	$(TMP_DIR)\xsAll.obj \
	$(TMP_DIR)\xsAPI.obj \
	$(TMP_DIR)\xsArguments.obj \
	$(TMP_DIR)\xsArray.obj \
	$(TMP_DIR)\xsAtomics.obj \
	$(TMP_DIR)\xsBigInt.obj \
	$(TMP_DIR)\xsBoolean.obj \
	$(TMP_DIR)\xsCode.obj \
	$(TMP_DIR)\xsCommon.obj \
	$(TMP_DIR)\xsDataView.obj \
	$(TMP_DIR)\xsDate.obj \
	$(TMP_DIR)\xsDebug.obj \
	$(TMP_DIR)\xsDefaults.obj \
	$(TMP_DIR)\xsError.obj \
	$(TMP_DIR)\xsFunction.obj \
	$(TMP_DIR)\xsGenerator.obj \
	$(TMP_DIR)\xsGlobal.obj \
	$(TMP_DIR)\xsJSON.obj \
	$(TMP_DIR)\xsLexical.obj \
	$(TMP_DIR)\xsLockdown.obj \
	$(TMP_DIR)\xsMapSet.obj \
	$(TMP_DIR)\xsMarshall.obj \
	$(TMP_DIR)\xsMath.obj \
	$(TMP_DIR)\xsMemory.obj \
	$(TMP_DIR)\xsModule.obj \
	$(TMP_DIR)\xsNumber.obj \
	$(TMP_DIR)\xsObject.obj \
	$(TMP_DIR)\xsPlatforms.obj \
	$(TMP_DIR)\xsProfile.obj \
	$(TMP_DIR)\xsPromise.obj \
	$(TMP_DIR)\xsProperty.obj \
	$(TMP_DIR)\xsProxy.obj \
	$(TMP_DIR)\xsRegExp.obj \
	$(TMP_DIR)\xsRun.obj \
	$(TMP_DIR)\xsScope.obj \
	$(TMP_DIR)\xsScript.obj \
	$(TMP_DIR)\xsSourceMap.obj \
	$(TMP_DIR)\xsString.obj \
	$(TMP_DIR)\xsSymbol.obj \
	$(TMP_DIR)\xsSyntaxical.obj \
	$(TMP_DIR)\xsTree.obj \
	$(TMP_DIR)\xsType.obj \
	$(TMP_DIR)\xsdtoa.obj \
	$(TMP_DIR)\xsre.obj \
	$(TMP_DIR)\api.obj \
	$(TMP_DIR)\dumper.obj \
	$(TMP_DIR)\emitter.obj \
	$(TMP_DIR)\loader.obj \
	$(TMP_DIR)\parser.obj \
	$(TMP_DIR)\reader.obj \
	$(TMP_DIR)\scanner.obj \
	$(TMP_DIR)\writer.obj \
	$(TMP_DIR)\xsmc.obj \
	$(TMP_DIR)\textdecoder.obj \
	$(TMP_DIR)\textencoder.obj \
	$(TMP_DIR)\modBase64.obj \
	$(TMP_DIR)\xst.obj

build : $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)\$(NAME).exe

$(TMP_DIR) :
	if not exist $(TMP_DIR)\$(NULL) mkdir $(TMP_DIR)

$(BIN_DIR) :
	if not exist $(BIN_DIR)\$(NULL) mkdir $(BIN_DIR)

$(BIN_DIR)\$(NAME).exe : $(OBJECTS)
	link \
		$(LINK_OPTIONS) \
		$(LIBRARIES) \
		$(OBJECTS) \
		/implib:$(TMP_DIR)\$(NAME).lib \
		/out:$(BIN_DIR)\$(NAME).exe

$(OBJECTS) : $(TLS_DIR)\xst.h
$(OBJECTS) : $(PLT_DIR)\xsPlatform.h
$(OBJECTS) : $(SRC_DIR)\xsCommon.h
$(OBJECTS) : $(SRC_DIR)\xsAll.h
$(OBJECTS) : $(SRC_DIR)\xsScript.h

{$(SRC_DIR)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(TLS_DIR)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(TLS_DIR)\yaml\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(MODDABLE)\modules\data\text\decoder\}.c{$(TMP_DIR)\}.obj:
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(MODDABLE)\modules\data\text\encoder\}.c{$(TMP_DIR)\}.obj:
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(MODDABLE)\modules\data\base64\}.c{$(TMP_DIR)\}.obj:
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)

clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).exe
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).exe
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)

