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
#

!IFNDEF GOAL
GOAL = debug
!ENDIF
NAME = simulator

!IFNDEF XS_DIR
XS_DIR = ..\..\..\xs
!ENDIF

!IFNDEF BUILD_DIR
BUILD_DIR = ..\..
!ENDIF

BIN_DIR = $(BUILD_DIR)\bin\win\$(GOAL)
TMP_DIR = $(BUILD_DIR)\tmp\win\$(GOAL)\$(NAME)

SIMULATOR = $(MODDABLE)/build/simulator

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/I$(SIMULATOR) \
	/I$(SIMULATOR)\win \
	/nologo \
	/Zp1 
!IF "$(GOAL)"=="debug"
C_OPTIONS = $(C_OPTIONS) \
	/D _DEBUG \
	/Fp$(TMP_DIR)\ \
	/Od \
	/W3 \
	/Z7
!ELSE
C_OPTIONS = $(C_OPTIONS) \
	/D NDEBUG \
	/Fp$(TMP_DIR)\ \
	/O2 \
	/W0
!ENDIF

RC_OPTIONS = /nologo

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib gdiplus.lib ole32.lib shell32.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo
!IF "$(GOAL)"=="debug"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

build : $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)\$(NAME).exe

$(TMP_DIR) :
	if not exist $(TMP_DIR)\$(NULL) mkdir $(TMP_DIR)

$(BIN_DIR) :
	if not exist $(BIN_DIR)\$(NULL) mkdir $(BIN_DIR)

$(BIN_DIR)\$(NAME).exe : $(TMP_DIR)\main.o $(TMP_DIR)\main.res
	link \
		$(LINK_OPTIONS) \
		$(LIBRARIES) \
		$(TMP_DIR)\main.o \
		$(TMP_DIR)\main.res \
		/implib:$(TMP_DIR)\$(NAME).lib \
		/out:$(BIN_DIR)\$(NAME).exe

$(TMP_DIR)\main.o : $(SIMULATOR)\win\main.cpp $(SIMULATOR)\screen.h
	cl $(SIMULATOR)\win\main.cpp $(C_OPTIONS) /Fo$@
	
$(TMP_DIR)\main.res : $(SIMULATOR)\win\main.rc
	rc $(RC_OPTIONS) /Fo$@ $(SIMULATOR)\win\main.rc
	
clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).exe
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).exe
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)
