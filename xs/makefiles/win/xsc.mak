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
# This file incorporates work covered by the following copyright and  
# permission notice:  
#
#       Copyright (C) 2010-2016 Marvell International Ltd.
#       Copyright (C) 2002-2010 Kinoma, Inc.
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#

!IFNDEF GOAL
GOAL = debug
!ENDIF
NAME = xsc

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
	-D mxCompile=1 \
	-D mxParse=1 \
	/I$(INC_DIR) \
	/I$(PLT_DIR) \
	/I$(SRC_DIR) \
	/I$(TLS_DIR) \
	/I$(TMP_DIR) \
	/nologo \
	/Zp1
	
!IF "$(GOAL)"=="debug"
C_OPTIONS = $(C_OPTIONS) \
	/D _DEBUG \
	/D mxDebug \
	/Fp$(TMP_DIR_DBG)\ \
	/Od \
	/W3 \
	/Z7
!ELSE
C_OPTIONS = $(C_OPTIONS) \
	/D NDEBUG \
	/Fp$(TMP_DIR_RLS)\ \
	/O2 \
	/W0
!ENDIF

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo /subsystem:console
!IF "$(GOAL)"=="debug"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

OBJECTS = \
	$(TMP_DIR)\xsdtoa.o \
	$(TMP_DIR)\xsre.o \
	$(TMP_DIR)\xsCommon.o \
	$(TMP_DIR)\xsScript.o \
	$(TMP_DIR)\xsLexical.o \
	$(TMP_DIR)\xsSyntaxical.o \
	$(TMP_DIR)\xsTree.o \
	$(TMP_DIR)\xsSourceMap.o \
	$(TMP_DIR)\xsScope.o \
	$(TMP_DIR)\xsCode.o \
	$(TMP_DIR)\xsc.o

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

$(OBJECTS) : $(PLT_DIR)\xsPlatform.h
$(OBJECTS) : $(SRC_DIR)\xsCommon.h
$(OBJECTS) : $(SRC_DIR)\xsScript.h

{$(SRC_DIR)\}.c{$(TMP_DIR)\}.o:
	cl $< $(C_OPTIONS) /Fo$@
{$(TLS_DIR)\}.c{$(TMP_DIR)\}.o:
	cl $< $(C_OPTIONS) /Fo$@

clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).exe
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).exe
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)

