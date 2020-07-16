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

BUILDCLUT = $(BUILD_DIR)\bin\win\debug\buildclut
COMPRESSBMF = $(BUILD_DIR)\bin\win\debug\compressbmf
IMAGE2CS = $(BUILD_DIR)\bin\win\debug\image2cs
MCLOCAL = $(BUILD_DIR)\bin\win\debug\mclocal
MCREZ = $(BUILD_DIR)\bin\win\debug\mcrez
PNG2BMP = $(BUILD_DIR)\bin\win\debug\png2bmp
RLE4ENCODE = $(BUILD_DIR)\bin\win\debug\rle4encode
WAV2MAUD = $(BUILD_DIR)\bin\win\debug\wav2maud
XSC = $(BUILD_DIR)\bin\win\debug\xsc
XSL = $(BUILD_DIR)\bin\win\debug\xsl

all: $(BIN_DIR)\mc.xsa
	start $(SIMULATOR) $(BIN_DIR)\mc.xsa

$(BIN_DIR)\mc.xsa: $(DATA) $(MODULES) $(RESOURCES)
	@echo # xsl mc.xsa
	$(XSL) -a -b $(MODULES_DIR) -n $(DOT_SIGNATURE) -o $(BIN_DIR) $(DATA) $(MODULES) $(RESOURCES)
