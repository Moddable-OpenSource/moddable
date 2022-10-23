#
# Copyright (c) 2016-2022  Moddable Tech, Inc.
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

% : %.c
%.o : %.c

GOAL ?= debug
NAME = tools
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

XS_DIR ?= $(realpath ../../../xs)
BUILD_DIR ?= $(realpath ../..)

COMMODETTO = $(MODDABLE)/modules/commodetto
CRYPT = $(MODDABLE)/modules/crypt
INSTRUMENTATION = $(MODDABLE)/modules/base/instrumentation
TOOLS = $(MODDABLE)/tools

BIN_DIR = $(BUILD_DIR)/bin/mac/$(GOAL)
LIB_DIR = $(BUILD_DIR)/tmp/mac/$(GOAL)/lib
TMP_DIR = $(BUILD_DIR)/tmp/mac/$(GOAL)/$(NAME)
MOD_DIR = $(TMP_DIR)/modules

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources \
	$(XS_DIR)/tools
	
XS_HEADERS = \
	$(XS_DIR)/platforms/mac_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h
	
XS_OBJECTS = \
	$(LIB_DIR)/mac_xs.c.o \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArguments.c.o \
	$(LIB_DIR)/xsArray.c.o \
	$(LIB_DIR)/xsAtomics.c.o \
	$(LIB_DIR)/xsBigInt.c.o \
	$(LIB_DIR)/xsBoolean.c.o \
	$(LIB_DIR)/xsCode.c.o \
	$(LIB_DIR)/xsCommon.c.o \
	$(LIB_DIR)/xsDataView.c.o \
	$(LIB_DIR)/xsDate.c.o \
	$(LIB_DIR)/xsDebug.c.o \
	$(LIB_DIR)/xsError.c.o \
	$(LIB_DIR)/xsFunction.c.o \
	$(LIB_DIR)/xsGenerator.c.o \
	$(LIB_DIR)/xsGlobal.c.o \
	$(LIB_DIR)/xsJSON.c.o \
	$(LIB_DIR)/xsLexical.c.o \
	$(LIB_DIR)/xsMapSet.c.o \
	$(LIB_DIR)/xsMarshall.c.o \
	$(LIB_DIR)/xsMath.c.o \
	$(LIB_DIR)/xsMemory.c.o \
	$(LIB_DIR)/xsModule.c.o \
	$(LIB_DIR)/xsNumber.c.o \
	$(LIB_DIR)/xsObject.c.o \
	$(LIB_DIR)/xsPlatforms.c.o \
	$(LIB_DIR)/xsProfile.c.o \
	$(LIB_DIR)/xsPromise.c.o \
	$(LIB_DIR)/xsProperty.c.o \
	$(LIB_DIR)/xsProxy.c.o \
	$(LIB_DIR)/xsRegExp.c.o \
	$(LIB_DIR)/xsRun.c.o \
	$(LIB_DIR)/xsScope.c.o \
	$(LIB_DIR)/xsScript.c.o \
	$(LIB_DIR)/xsSourceMap.c.o \
	$(LIB_DIR)/xsString.c.o \
	$(LIB_DIR)/xsSymbol.c.o \
	$(LIB_DIR)/xsSyntaxical.c.o \
	$(LIB_DIR)/xsTree.c.o \
	$(LIB_DIR)/xsType.c.o \
	$(LIB_DIR)/xsdtoa.c.o \
	$(LIB_DIR)/xsmc.c.o \
	$(LIB_DIR)/xsre.c.o \
	$(LIB_DIR)/xsa.c.o \
	$(LIB_DIR)/xsc.c.o \
	$(LIB_DIR)/xslBase.c.o

MODULES = \
	$(MOD_DIR)/commodetto/Bitmap.xsb \
	$(MOD_DIR)/commodetto/BMPOut.xsb \
	$(MOD_DIR)/commodetto/BufferOut.xsb \
	$(MOD_DIR)/commodetto/ColorCellOut.xsb \
	$(MOD_DIR)/commodetto/Convert.xsb \
	$(MOD_DIR)/commodetto/ParseBMF.xsb \
	$(MOD_DIR)/commodetto/ParseBMP.xsb \
	$(MOD_DIR)/commodetto/PixelsOut.xsb \
	$(MOD_DIR)/commodetto/Poco.xsb \
	$(MOD_DIR)/commodetto/ReadJPEG.xsb \
	$(MOD_DIR)/commodetto/ReadPNG.xsb \
	$(MOD_DIR)/commodetto/RLE4Out.xsb \
	$(MOD_DIR)/ber.xsb \
	$(MOD_DIR)/crypt.xsb \
	$(MOD_DIR)/curve.xsb \
	$(MOD_DIR)/ec.xsb \
	$(MOD_DIR)/ecp.xsb \
	$(MOD_DIR)/modular.xsb \
	$(MOD_DIR)/x509.xsb \
	$(MOD_DIR)/file.xsb \
	$(MOD_DIR)/buildclut.xsb \
	$(MOD_DIR)/cdv.xsb \
	$(MOD_DIR)/colorcellencode.xsb \
	$(MOD_DIR)/compileDataView.xsb \
	$(MOD_DIR)/compressbmf.xsb \
	$(MOD_DIR)/image2cs.xsb \
	$(MOD_DIR)/mcbundle.xsb \
	$(MOD_DIR)/mcconfig.xsb \
	$(MOD_DIR)/mclocal.xsb \
	$(MOD_DIR)/mcmanifest.xsb \
	$(MOD_DIR)/mcprintski.xsb \
	$(MOD_DIR)/mcrez.xsb \
	$(MOD_DIR)/nodered2mcu.xsb \
	$(MOD_DIR)/png2bmp.xsb \
	$(MOD_DIR)/resampler.xsb \
	$(MOD_DIR)/rle4encode.xsb \
	$(MOD_DIR)/tool.xsb \
	$(MOD_DIR)/unicode-ranges.xsb \
	$(MOD_DIR)/wav2maud.xsb \
	$(MOD_DIR)/bles2gatt.xsb \
	$(TMP_DIR)/commodettoBitmap.c.xsi \
	$(TMP_DIR)/commodettoBufferOut.c.xsi \
	$(TMP_DIR)/commodettoColorCellOut.c.xsi \
	$(TMP_DIR)/commodettoConvert.c.xsi \
	$(TMP_DIR)/commodettoParseBMF.c.xsi \
	$(TMP_DIR)/commodettoParseBMP.c.xsi \
	$(TMP_DIR)/commodettoPoco.c.xsi \
	$(TMP_DIR)/commodettoPocoBlit.c.xsi \
	$(TMP_DIR)/commodettoReadJPEG.c.xsi \
	$(TMP_DIR)/commodettoReadPNG.c.xsi \
	$(TMP_DIR)/cfeBMF.c.xsi \
	$(TMP_DIR)/xsBigIntEx.c.xsi \
	$(TMP_DIR)/ec.c.xsi \
	$(TMP_DIR)/image2cs.c.xsi \
	$(TMP_DIR)/miniz.c.xsi \
	$(TMP_DIR)/modCrypt.c.xsi \
	$(TMP_DIR)/modInstrumentation.c.xsi \
	$(TMP_DIR)/modular.c.xsi \
	$(TMP_DIR)/mont.c.xsi \
	$(TMP_DIR)/tool.c.xsi \
	$(TMP_DIR)/x509.c.xsi
PRELOADS =\
	-p commodetto/Bitmap.xsb\
	-p commodetto/BMPOut.xsb\
	-p commodetto/BufferOut.xsb\
	-p commodetto/ColorCellOut.xsb\
	-p commodetto/Convert.xsb\
	-p commodetto/ParseBMF.xsb\
	-p commodetto/ParseBMP.xsb\
	-p commodetto/Poco.xsb\
	-p commodetto/ReadPNG.xsb\
	-p commodetto/RLE4Out.xsb\
	-p ber.xsb\
	-p crypt.xsb\
	-p curve.xsb\
	-p ec.xsb\
	-p ecp.xsb\
	-p modular.xsb\
	-p modular.xsb\
	-p x509.xsb\
	-p resampler.xsb\
	-p unicode-ranges.xsb\
	-p file.xsb
CREATION = -c 134217728,16777216,8388608,1048576,16384,16384,1993,127,32768,1993,0,main

HEADERS = \
	$(COMMODETTO)/commodettoBitmap.h \
	$(COMMODETTO)/commodettoPocoBlit.h \
	$(INSTRUMENTATION)/modInstrumentation.h
OBJECTS = \
	$(TMP_DIR)/adpcm-lib.c.o \
	$(TMP_DIR)/commodettoBitmap.c.o \
	$(TMP_DIR)/commodettoBufferOut.c.o \
	$(TMP_DIR)/commodettoColorCellOut.c.o \
	$(TMP_DIR)/commodettoConvert.c.o \
	$(TMP_DIR)/commodettoParseBMF.c.o \
	$(TMP_DIR)/commodettoParseBMP.c.o \
	$(TMP_DIR)/commodettoPoco.c.o \
	$(TMP_DIR)/commodettoPocoBlit.c.o \
	$(TMP_DIR)/commodettoReadJPEG.c.o \
	$(TMP_DIR)/commodettoReadPNG.c.o \
	$(TMP_DIR)/cfeBMF.c.o \
	$(TMP_DIR)/xsBigIntEx.c.o \
	$(TMP_DIR)/ec.c.o \
	$(TMP_DIR)/image2cs.c.o \
	$(TMP_DIR)/miniz.c.o \
	$(TMP_DIR)/modCrypt.c.o \
	$(TMP_DIR)/modInstrumentation.c.o \
	$(TMP_DIR)/modular.c.o \
	$(TMP_DIR)/mont.c.o \
	$(TMP_DIR)/tool.c.o \
	$(TMP_DIR)/wav2maud.c.o \
	$(TMP_DIR)/x509.c.o \
	$(TMP_DIR)/chacha.c.o \
	$(TMP_DIR)/fips46.c.o \
	$(TMP_DIR)/fips180.c.o \
	$(TMP_DIR)/fips197.c.o \
	$(TMP_DIR)/ghash.c.o \
	$(TMP_DIR)/rc.c.o \
	$(TMP_DIR)/rfc1321.c.o

COMMANDS = \
	$(BIN_DIR)/buildclut \
	$(BIN_DIR)/cdv \
	$(BIN_DIR)/colorcellencode \
	$(BIN_DIR)/compressbmf \
	$(BIN_DIR)/image2cs \
	$(BIN_DIR)/mcbundle \
	$(BIN_DIR)/mcconfig \
	$(BIN_DIR)/mclocal \
	$(BIN_DIR)/mcprintski \
	$(BIN_DIR)/mcrez \
	$(BIN_DIR)/nodered2mcu \
	$(BIN_DIR)/png2bmp \
	$(BIN_DIR)/rle4encode \
	$(BIN_DIR)/wav2maud \
	$(BIN_DIR)/bles2gatt

ifeq ($(wildcard $(TOOLS)/mcrun.js),) 
else 
  MODULES += $(MOD_DIR)/mcrun.xsb
  COMMANDS += $(BIN_DIR)/mcrun
endif 

C_DEFINES = \
	-DXS_ARCHIVE=1 \
	-DINCLUDE_XSPLATFORM=1 \
	-DXSPLATFORM=\"mac_xs.h\" \
	-DXSTOOLS=1 \
	-DmxRun=1 \
	-DmxParse=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1
ifeq ($(GOAL),debug)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(INSTRUMENTATION) $(COMMODETTO) ${CRYPT}/etc ${CRYPT}/arith ${CRYPT}/digest ${CRYPT}/digest/kcl $(TOOLS) $(TMP_DIR),-I$(dir))
# C_FLAGS = -c -arch i386
C_FLAGS = -c
ifeq ($(GOAL),debug)
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
	C_FLAGS += -D_RELEASE=1 -O3
endif

LIBRARIES = -framework CoreServices

# LINK_FLAGS = -arch i386
LINK_FLAGS =

XSC = $(BUILD_DIR)/bin/mac/$(GOAL)/xsc
XSID = $(BUILD_DIR)/bin/mac/$(GOAL)/xsid
XSL = $(BUILD_DIR)/bin/mac/$(GOAL)/xsl
	
VPATH += $(XS_DIRECTORIES) $(COMMODETTO) $(INSTRUMENTATION) $(CRYPT)/etc $(CRYPT)/digest ${CRYPT}/digest/kcl ${CRYPT}/arith $(TOOLS)

build: $(LIB_DIR) $(TMP_DIR) $(MOD_DIR) $(MOD_DIR)/commodetto $(MOD_DIR) $(BIN_DIR) $(BIN_DIR)/$(NAME) $(COMMANDS) $(BIN_DIR)/README.txt

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
$(TMP_DIR):
	mkdir -p $(TMP_DIR)
	
$(MOD_DIR):
	mkdir -p $(MOD_DIR)
	
$(MOD_DIR)/commodetto:
	mkdir -p $(MOD_DIR)/commodetto
	
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(NAME): $(XS_OBJECTS) $(OBJECTS) $(TMP_DIR)/mc.xs.c.o
	@echo "#" $(NAME) $(GOAL) ": cc" $(@F)
	$(CC) $(LINK_FLAGS) $(LIBRARIES) $(XS_OBJECTS) $(OBJECTS) $(TMP_DIR)/mc.xs.c.o -o $@

$(XS_OBJECTS) : $(XS_HEADERS)
$(LIB_DIR)/%.c.o: %.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c.o: $(TMP_DIR)/mc.xs.c $(HEADERS)
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES)
	@echo "#" $(NAME) $(GOAL) ": xsl modules"
	$(XSL) -b $(MOD_DIR) -o $(TMP_DIR) $(PRELOADS) $(CREATION) $(MODULES)

$(MOD_DIR)/commodetto/%.xsb: $(COMMODETTO)/commodetto%.js
	@echo "#" $(NAME) $(GOAL) ": xsc" $(<F)
	$(BIN_DIR)/xsc $< -c -d -e -o $(MOD_DIR)/commodetto -r $*

$(MOD_DIR)/%.xsb: $(CRYPT)/arith/%.js
	@echo "#" $(NAME) $(GOAL) ": xsc" $(<F)
	$(BIN_DIR)/xsc $< -c -d -e -o $(MOD_DIR) -r $*

$(MOD_DIR)/%.xsb: $(CRYPT)/digest/%.js
	@echo "#" $(NAME) $(GOAL) ": xsc" $(<F)
	$(BIN_DIR)/xsc $< -c -d -e -o $(MOD_DIR) -r $*

$(MOD_DIR)/%.xsb: $(CRYPT)/etc/%.js
	@echo "#" $(NAME) $(GOAL) ": xsc" $(<F)
	$(BIN_DIR)/xsc $< -c -d -e -o $(MOD_DIR) -r $*

$(MOD_DIR)/%.xsb: $(TOOLS)/%.js
	@echo "#" $(NAME) $(GOAL) ": xsc" $(<F)
	$(BIN_DIR)/xsc -c -d -e $< -o $(MOD_DIR)

$(OBJECTS): $(XS_HEADERS) $(HEADERS) | $(TMP_DIR)/mc.xs.c
$(TMP_DIR)/%.c.o: %.c
	@echo "#" $(NAME) $(GOAL) ": cc" $(<F)
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -c -o $@

$(TMP_DIR)/%.c.xsi: %.c
	@echo "#" $(NAME) $(GOAL) ": xsid" $(<F)
	$(XSID) $< -o $(TMP_DIR)

$(BIN_DIR)/buildclut: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": buildclut"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools buildclut "$$@"' > $(BIN_DIR)/buildclut
	chmod +x $(BIN_DIR)/buildclut

$(BIN_DIR)/cdv: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": cdv"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools cdv "$$@"' > $(BIN_DIR)/cdv
	chmod +x $(BIN_DIR)/cdv

$(BIN_DIR)/colorcellencode: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": colorcellencode"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools colorcellencode "$$@"' > $(BIN_DIR)/colorcellencode
	chmod +x $(BIN_DIR)/colorcellencode

$(BIN_DIR)/compressbmf: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": compressbmf"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools compressbmf "$$@"' > $(BIN_DIR)/compressbmf
	chmod +x $(BIN_DIR)/compressbmf

$(BIN_DIR)/image2cs: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": image2cs"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools image2cs "$$@"' > $(BIN_DIR)/image2cs
	chmod +x $(BIN_DIR)/image2cs

$(BIN_DIR)/mcbundle: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mcbundle"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mcbundle "$$@"' > $(BIN_DIR)/mcbundle
	chmod +x $(BIN_DIR)/mcbundle

$(BIN_DIR)/mcconfig: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mcconfig"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mcconfig "$$@"' > $(BIN_DIR)/mcconfig
	chmod +x $(BIN_DIR)/mcconfig

$(BIN_DIR)/mclocal: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mclocal"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mclocal "$$@"' > $(BIN_DIR)/mclocal
	chmod +x $(BIN_DIR)/mclocal

$(BIN_DIR)/mcprintski: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mcprintski"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mcprintski "$$@"' > $(BIN_DIR)/mcprintski
	chmod +x $(BIN_DIR)/mcprintski

$(BIN_DIR)/mcrez: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mcrez"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mcrez "$$@"' > $(BIN_DIR)/mcrez
	chmod +x $(BIN_DIR)/mcrez

$(BIN_DIR)/mcrun: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": mcrun"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools mcrun "$$@"' > $(BIN_DIR)/mcrun
	chmod +x $(BIN_DIR)/mcrun

$(BIN_DIR)/nodered2mcu: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": nodered2mcu"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools nodered2mcu "$$@"' > $(BIN_DIR)/nodered2mcu
	chmod +x $(BIN_DIR)/nodered2mcu

$(BIN_DIR)/png2bmp: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": png2bmp"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools png2bmp "$$@"' > $(BIN_DIR)/png2bmp
	chmod +x $(BIN_DIR)/png2bmp

$(BIN_DIR)/rle4encode: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": rle4encode"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools rle4encode "$$@"' > $(BIN_DIR)/rle4encode
	chmod +x $(BIN_DIR)/rle4encode

$(BIN_DIR)/wav2maud: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": wav2maud"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools wav2maud "$$@"' > $(BIN_DIR)/wav2maud
	chmod +x $(BIN_DIR)/wav2maud

$(BIN_DIR)/bles2gatt: $(MAKEFILE_LIST)
	@echo "#" $(NAME) $(GOAL) ": bles2gatt"
	echo '#!/bin/bash\nDIR=$$(cd "$$(dirname "$$BASH_SOURCE")"; cd -P "$$(dirname "$$(readlink "$$BASH_SOURCE" || echo .)")"; pwd)\n$$DIR/tools bles2gatt "$$@"' > $(BIN_DIR)/bles2gatt
	chmod +x $(BIN_DIR)/bles2gatt

$(BIN_DIR)/README.txt:
	@echo "Note: If you downloaded these binaries, macOS may display an error when you try to run them.\n\nExecute the following commands to allow the binaries to run.\n\n\txattr -d com.apple.quarantine xsbug.app\n\txattr -d com.apple.quarantine mcsim.app\n\txattr -d com.apple.quarantine tools\n\txattr -d com.apple.quarantine serial2xsbug\n\txattr -d com.apple.quarantine xst\n\txattr -d com.apple.quarantine xsc\n\txattr -d com.apple.quarantine xsl\n\txattr -d com.apple.quarantine xsid\n" > $(BIN_DIR)/README.txt

clean:
	rm -rf $(BUILD_DIR)/bin/mac/debug/$(NAME).*
	rm -rf $(BUILD_DIR)/bin/mac/release/$(NAME).*
	rm -rf $(BUILD_DIR)/tmp/mac/debug/$(NAME)
	rm -rf $(BUILD_DIR)/tmp/mac/release/$(NAME)
