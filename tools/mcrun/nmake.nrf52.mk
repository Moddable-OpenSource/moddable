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

!IF "$(USE_USB)"==""
USE_USB = 0
!ELSE
!IF "$(M4_VID)"==""
M4_VID = BEEF
!ENDIF
!IF "$(M4_PID)"==""
M4_PID = CAFE
!ENDIF
!ENDIF

!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 921600
!ENDIF

ARCHIVE = $(BIN_DIR)\$(NAME).xsa

!IF "$(DEBUG)"=="1"
LAUNCH = debug
!ELSE
LAUNCH = release
!ENDIF

SCRIPT_PATH = $(MODDABLE)\build\devices\nrf52\config

KILL_SERIAL2XSBUG = -tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
START_XSBUG= tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start xsbug.exe && echo Starting xsbug... && timeout /nobreak /t 7 > nul)

DO_MOD_UPLOAD = echo "Running serial2xsbug..." && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && serial2xsbug $(UPLOAD_PORT) $(DEBUGGER_SPEED) 8N1 -install $(ARCHIVE)

!IF "$(USE_USB)"=="0"
!IF "$(UPLOAD_PORT)"==""
DO_MOD_UPLOAD = echo "\#\#\# No port. Set UPLOAD_PORT"; exit 1
!ELSE

!ENDIF
!ELSE
!IF [$(SCRIPT_PATH)\nrfSerialPort.bat $(M4_VID) $(M4_PID) $(TMP_DIR)\_default_port.tmp 2> null] == 0
UPLOAD_PORT = \
!INCLUDE $(TMP_DIR)\_default_port.tmp
!ENDIF
!ENDIF


all: $(LAUNCH)
	
debug: $(ARCHIVE)
	$(KILL_SERIAL2XSBUG)
	$(START_XSBUG)
	$(DO_MOD_UPLOAD)

release: $(ARCHIVE)
	$(KILL_SERIAL2XSBUG)
	$(DO_MOD_UPLOAD)

$(ARCHIVE): $(DATA) $(MODULES) $(RESOURCES)
	@echo "# xsl "$(NAME)".xsa"
	xsl -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) -r $(NAME) -u / $(DATA) $(MODULES) $(RESOURCES)
