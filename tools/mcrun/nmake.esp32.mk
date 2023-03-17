#
# Copyright (c) 2016-2023  Moddable Tech, Inc.
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

PROJ_DIR_TEMPLATE = $(BUILD_DIR)\devices\esp32\xsProj-$(ESP32_SUBCLASS)

!IF "$(UPLOAD_PORT)"==""
!IF [python $(PROJ_DIR_TEMPLATE)\getPort.py $(IDF_PATH)\tools > $(TMP_DIR)\_default_port.tmp 2> nul] == 0
DEFAULT_PORT = \
!INCLUDE $(TMP_DIR)\_default_port.tmp
!IF [del $(TMP_DIR)\_default_port.tmp] == 0
!ENDIF
!ENDIF
UPLOAD_PORT = $(DEFAULT_PORT)
!ENDIF

!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 460800
!ENDIF

MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\release
BUILDCLUT = $(MODDABLE_TOOLS_DIR)\buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)\compressbmf
IMAGE2CS = $(MODDABLE_TOOLS_DIR)\image2cs
MCLOCAL = $(MODDABLE_TOOLS_DIR)\mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)\mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)\png2bmp
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)\rle4encode
SERIAL2XSBUG = $(MODDABLE_TOOLS_DIR)\serial2xsbug
WAV2MAUD = $(MODDABLE_TOOLS_DIR)\wav2maud
XSBUG = $(MODDABLE_TOOLS_DIR)\xsbug
XSC = $(MODDABLE_TOOLS_DIR)\xsc
XSL = $(MODDABLE_TOOLS_DIR)\xsl

ARCHIVE = $(BIN_DIR)\$(NAME).xsa

!IF "$(DEBUG)"=="1"
LAUNCH = debug
!ELSE
LAUNCH = release
!ENDIF

all: $(LAUNCH)
	
debug: $(ARCHIVE)
	-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
	tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(XSBUG).exe && echo Starting xsbug... && timeout /nobreak /t 7 > nul)
	echo "Running serial2xsbug..." && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)
	
release: $(ARCHIVE)
	echo "Running serial2xsbug..." && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && $(SERIAL2XSBUG) $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) -u / $(DATA) $(MODULES) $(RESOURCES)
