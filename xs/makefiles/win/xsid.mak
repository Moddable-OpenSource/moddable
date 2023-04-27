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
NAME = xsid

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
	/I$(INC_DIR) \
	/I$(PLT_DIR) \
	/I$(SRC_DIR) \
	/I$(TLS_DIR) \
	/I$(TMP_DIR) \
	/nologo \
	/MP
	
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
	
LINK_OPTIONS = /incremental:no /nologo /subsystem:console
!IF "$(GOAL)"=="debug"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

OBJECTS = \
	$(TMP_DIR)\xsdtoa.obj \
	$(TMP_DIR)\xsre.obj \
	$(TMP_DIR)\xsCommon.obj \
	$(TMP_DIR)\xsid.obj

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

{$(SRC_DIR)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(TLS_DIR)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)

clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).exe
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).exe
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)

