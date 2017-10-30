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

.SUFFIXES : .js

!IFNDEF GOAL
GOAL = debug
!ENDIF
NAME = tools

!IFNDEF XS_DIR
XS_DIR = ..\..\..\xs
!ENDIF

!IFNDEF BUILD_DIR
BUILD_DIR = ..\..
!ENDIF

COMMODETTO = $(MODDABLE)\modules\commodetto
INSTRUMENTATION = $(MODDABLE)\modules\base\instrumentation
TOOLS = $(MODDABLE)\tools

BIN_DIR = $(BUILD_DIR)\bin\win\$(GOAL)
LIB_DIR = $(BUILD_DIR)\tmp\win\$(GOAL)\lib
TMP_DIR = $(BUILD_DIR)\tmp\win\$(GOAL)\$(NAME)
MOD_DIR = $(TMP_DIR)\modules

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
	$(LIB_DIR)\xspcre.o

MODULES = \
	$(MOD_DIR)\commodetto\Bitmap.xsb \
	$(MOD_DIR)\commodetto\BMPOut.xsb \
	$(MOD_DIR)\commodetto\ColorCellOut.xsb \
	$(MOD_DIR)\commodetto\Convert.xsb \
	$(MOD_DIR)\commodetto\ParseBMF.xsb \
	$(MOD_DIR)\commodetto\ParseBMP.xsb \
	$(MOD_DIR)\commodetto\PixelsOut.xsb \
	$(MOD_DIR)\commodetto\Poco.xsb \
	$(MOD_DIR)\commodetto\ReadJPEG.xsb \
	$(MOD_DIR)\commodetto\ReadPNG.xsb \
	$(MOD_DIR)\commodetto\RLE4Out.xsb \
	$(MOD_DIR)\file.xsb \
	$(MOD_DIR)\fs.xsb \
	$(MOD_DIR)\buildclut.xsb \
	$(MOD_DIR)\colorcellencode.xsb \
	$(MOD_DIR)\compressbmf.xsb \
	$(MOD_DIR)\image2cs.xsb \
	$(MOD_DIR)\mcconfig.xsb \
	$(MOD_DIR)\mclocal.xsb \
	$(MOD_DIR)\mcmanifest.xsb \
	$(MOD_DIR)\mcrez.xsb \
	$(MOD_DIR)\png2bmp.xsb \
	$(MOD_DIR)\rle4encode.xsb \
	$(MOD_DIR)\tool.xsb \
	$(TMP_DIR)\commodettoBitmap.xsi \
	$(TMP_DIR)\commodettoColorCellOut.xsi \
	$(TMP_DIR)\commodettoConvert.xsi \
	$(TMP_DIR)\commodettoPoco.xsi \
	$(TMP_DIR)\commodettoPocoBlit.xsi \
	$(TMP_DIR)\commodettoParseBMP.xsi \
	$(TMP_DIR)\commodettoParseBMF.xsi \
	$(TMP_DIR)\commodettoReadJPEG.xsi \
	$(TMP_DIR)\commodettoReadPNG.xsi \
	$(TMP_DIR)\miniz.xsi \
	$(TMP_DIR)\modInstrumentation.xsi \
	$(TMP_DIR)\fs.xsi \
	$(TMP_DIR)\tool.xsi \
	$(TMP_DIR)\main.xsi \
	$(TMP_DIR)\image2cs.xsi
PRELOADS =\
	-p commodetto\Bitmap.xsb\
	-p commodetto\BMPOut.xsb\
	-p commodetto\ColorCellOut.xsb\
	-p commodetto\Convert.xsb\
	-p commodetto\ParseBMF.xsb\
	-p commodetto\ParseBMP.xsb\
	-p commodetto\Poco.xsb\
	-p commodetto\ReadPNG.xsb\
	-p commodetto\RLE4Out.xsb\
	-p file.xsb
CREATION = -c 134217728,16777216,8388608,1048576,16384,16384,1993,127,main

HEADERS =\
	$(COMMODETTO)\commodettoBitmap.h\
	$(COMMODETTO)\commodettoPocoBlit.h\
	$(INSTRUMENTATION)\modInstrumentation.h
OBJECTS = \
	$(TMP_DIR)\commodettoBitmap.o \
	$(TMP_DIR)\commodettoColorCellOut.o \
	$(TMP_DIR)\commodettoConvert.o \
	$(TMP_DIR)\commodettoParseBMP.o \
	$(TMP_DIR)\commodettoParseBMF.o \
	$(TMP_DIR)\commodettoPoco.o \
	$(TMP_DIR)\commodettoPocoBlit.o \
	$(TMP_DIR)\commodettoReadJPEG.o \
	$(TMP_DIR)\commodettoReadPNG.o \
	$(TMP_DIR)\miniz.o \
	$(TMP_DIR)\modInstrumentation.o \
	$(TMP_DIR)\fs.o \
	$(TMP_DIR)\tool.o \
	$(TMP_DIR)\main.o \
	$(TMP_DIR)\image2cs.o

COMMANDS = \
	$(BIN_DIR)\buildclut.bat \
	$(BIN_DIR)\colorcellencode.bat \
	$(BIN_DIR)\compressbmf.bat \
	$(BIN_DIR)\image2cs.bat \
	$(BIN_DIR)\mcconfig.bat \
	$(BIN_DIR)\mclocal.bat \
	$(BIN_DIR)\mcrez.bat \
	$(BIN_DIR)\png2bmp.bat \
	$(BIN_DIR)\rle4encode.bat

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/D INCLUDE_XSPLATFORM=1 \
	/D XSPLATFORM=\"win_xs.h\" \
	/D mxRun=1 \
	/D mxParse=1 \
	/D mxNoFunctionLength=1 \
	/D mxNoFunctionName=1 \
	/D mxHostFunctionPrimitive=1 \
	/D mxFewGlobalsTable=1 \
	/I$(XS_DIR)\includes \
	/I$(XS_DIR)\platforms \
	/I$(XS_DIR)\sources \
	/I$(XS_DIR)\sources\pcre \
	/I$(COMMODETTO) \
	/I$(INSTRUMENTATION) \
	/I$(TOOLS) \
	/I$(TMP_DIR) \
	/nologo \
	/Zp1 
	
!IF "$(GOAL)"=="debug"
C_OPTIONS = $(C_OPTIONS) \
	/D _DEBUG \
	/D MODINSTRUMENTATION=1 \
	/D mxDebug \
	/D mxInstrument=1 \
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

LIBRARIES = ws2_32.lib advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib user32.lib Iphlpapi.lib
	
LINK_OPTIONS = /incremental:no /machine:I386 /nologo /subsystem:console
!IF "$(GOAL)"=="debug"
LINK_OPTIONS = $(LINK_OPTIONS) /debug
!ENDIF

build : $(LIB_DIR) $(TMP_DIR) $(MOD_DIR) $(MOD_DIR)\commodetto $(BIN_DIR) $(BIN_DIR)\tools.exe $(COMMANDS)

$(LIB_DIR) :
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

$(TMP_DIR) :
	if not exist $(TMP_DIR)\$(NULL) mkdir $(TMP_DIR)

$(MOD_DIR) :
	if not exist $(MOD_DIR)\$(NULL) mkdir $(MOD_DIR)

$(MOD_DIR)\commodetto :
	if not exist $(MOD_DIR)\commodetto\$(NULL) mkdir $(MOD_DIR)\commodetto

$(BIN_DIR) :
	if not exist $(BIN_DIR)\$(NULL) mkdir $(BIN_DIR)

$(BIN_DIR)\tools.exe : $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(OBJECTS)
	link \
		$(LINK_OPTIONS) \
		$(LIBRARIES) \
		$(XS_OBJECTS) \
		$(TMP_DIR)\mc.xs.o \
		$(OBJECTS) \
		/implib:$(TMP_DIR)\$(NAME).lib \
		/out:$(BIN_DIR)\tools.exe

$(XS_OBJECTS) : $(XS_HEADERS)
{$(XS_DIR)\platforms\}.c{$(LIB_DIR)\}.o:
	@echo # cl $(<F)
	cl $(C_OPTIONS) $< /Fo$@
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # cl $(<F)
	cl $(C_OPTIONS) $< /Fo$@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c $(HEADERS)
	@echo # cl mc.xs.o
	cl $(C_OPTIONS) $(TMP_DIR)\mc.xs.c /Fo$@
$(TMP_DIR)\mc.xs.c: $(MODULES)
	@echo # xsl modules
	$(BIN_DIR)\xsl -b $(MOD_DIR) -o $(TMP_DIR) $(PRELOADS) $(CREATION) $(MODULES)
$(MOD_DIR)\commodetto\Bitmap.xsb : $(COMMODETTO)\commodettoBitmap.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\BMPOut.xsb : $(COMMODETTO)\commodettoBMPOut.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\ColorCellOut.xsb : $(COMMODETTO)\commodettoColorCellOut.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\Convert.xsb : $(COMMODETTO)\commodettoConvert.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\ParseBMF.xsb : $(COMMODETTO)\commodettoParseBMF.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\ParseBMP.xsb : $(COMMODETTO)\commodettoParseBMP.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\PixelsOut.xsb : $(COMMODETTO)\commodettoPixelsOut.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\Poco.xsb : $(COMMODETTO)\commodettoPoco.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\ReadJPEG.xsb : $(COMMODETTO)\commodettoReadJPEG.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\ReadPNG.xsb : $(COMMODETTO)\commodettoReadPNG.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
$(MOD_DIR)\commodetto\RLE4Out.xsb : $(COMMODETTO)\commodettoRLE4Out.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR)\commodetto -r $(@B)
{$(TOOLS)\}.js{$(MOD_DIR)\}.xsb:
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $< -c -d -e -o $(MOD_DIR)
	
{$(COMMODETTO)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(**F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR)
{$(INSTRUMENTATION)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(**F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR)
{$(TOOLS)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(**F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR)

$(OBJECTS) : $(XS_HEADERS) $(HEADERS)
{$(COMMODETTO)\}.c{$(TMP_DIR)\}.o:
	@echo # cl $(<F)
	cl $< $(C_OPTIONS) /Fo$@
{$(INSTRUMENTATION)\}.c{$(TMP_DIR)\}.o:
	@echo # cl $(<F)
	cl $< $(C_OPTIONS) /Fo$@
{$(TOOLS)\}.c{$(TMP_DIR)\}.o:
	@echo # cl $(<F)
	cl $< $(C_OPTIONS) /Fo$@
{$(TMP_DIR)\}.c{$(TMP_DIR)\}.o:
	@echo # cl $(<F)
	cl $< $(C_OPTIONS) /Fo$@

$(BIN_DIR)\buildclut.bat :
	echo @$(BIN_DIR)\tools buildclut %%* 1> $(BIN_DIR)\buildclut.bat
$(BIN_DIR)\colorcellencode.bat :
	echo @$(BIN_DIR)\tools colorcellencode %%* 1> $(BIN_DIR)\colorcellencode.bat
$(BIN_DIR)\compressbmf.bat :
	echo @$(BIN_DIR)\tools compressbmf %%* 1> $(BIN_DIR)\compressbmf.bat
$(BIN_DIR)\image2cs.bat :
	echo @$(BIN_DIR)\tools image2cs %%* 1> $(BIN_DIR)\image2cs.bat
$(BIN_DIR)\mcconfig.bat :
	echo @$(BIN_DIR)\tools mcconfig %%* 1> $(BIN_DIR)\mcconfig.bat
$(BIN_DIR)\mclocal.bat :
	echo @$(BIN_DIR)\tools mclocal %%* 1> $(BIN_DIR)\mclocal.bat
$(BIN_DIR)\mcrez.bat :
	echo @$(BIN_DIR)\tools mcrez %%* 1> $(BIN_DIR)\mcrez.bat
$(BIN_DIR)\png2bmp.bat :
	echo @$(BIN_DIR)\tools png2bmp %%* 1> $(BIN_DIR)\png2bmp.bat
$(BIN_DIR)\rle4encode.bat :
	echo @$(BIN_DIR)\tools rle4encode %%* 1> $(BIN_DIR)\rle4encode.bat

clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).*
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).*
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)

