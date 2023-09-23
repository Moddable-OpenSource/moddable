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

TOOLS_VERSION = \
!INCLUDE $(MODDABLE)\tools\VERSION

COMMODETTO = $(MODDABLE)\modules\commodetto
CRYPT = $(MODDABLE)\modules\crypt
DATA = $(MODDABLE)\modules\data
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
	$(LIB_DIR)\win_xs.obj \
	$(LIB_DIR)\xsAll.obj \
	$(LIB_DIR)\xsAPI.obj \
	$(LIB_DIR)\xsArguments.obj \
	$(LIB_DIR)\xsArray.obj \
	$(LIB_DIR)\xsAtomics.obj \
	$(LIB_DIR)\xsBigInt.obj \
	$(LIB_DIR)\xsBoolean.obj \
	$(LIB_DIR)\xsCode.obj \
	$(LIB_DIR)\xsCommon.obj \
	$(LIB_DIR)\xsDataView.obj \
	$(LIB_DIR)\xsDate.obj \
	$(LIB_DIR)\xsDebug.obj \
	$(LIB_DIR)\xsError.obj \
	$(LIB_DIR)\xsFunction.obj \
	$(LIB_DIR)\xsGenerator.obj \
	$(LIB_DIR)\xsGlobal.obj \
	$(LIB_DIR)\xsJSON.obj \
	$(LIB_DIR)\xsLexical.obj \
	$(LIB_DIR)\xsMapSet.obj \
	$(LIB_DIR)\xsMarshall.obj \
	$(LIB_DIR)\xsMath.obj \
	$(LIB_DIR)\xsMemory.obj \
	$(LIB_DIR)\xsModule.obj \
	$(LIB_DIR)\xsNumber.obj \
	$(LIB_DIR)\xsObject.obj \
	$(LIB_DIR)\xsPlatforms.obj \
	$(LIB_DIR)\xsProfile.obj \
	$(LIB_DIR)\xsPromise.obj \
	$(LIB_DIR)\xsProperty.obj \
	$(LIB_DIR)\xsProxy.obj \
	$(LIB_DIR)\xsRegExp.obj \
	$(LIB_DIR)\xsRun.obj \
	$(LIB_DIR)\xsScope.obj \
	$(LIB_DIR)\xsScript.obj \
	$(LIB_DIR)\xsSourceMap.obj \
	$(LIB_DIR)\xsString.obj \
	$(LIB_DIR)\xsSymbol.obj \
	$(LIB_DIR)\xsSyntaxical.obj \
	$(LIB_DIR)\xsTree.obj \
	$(LIB_DIR)\xsType.obj \
	$(LIB_DIR)\xsdtoa.obj \
	$(LIB_DIR)\xsmc.obj \
	$(LIB_DIR)\xsre.obj \
	$(LIB_DIR)\xsa.obj \
	$(LIB_DIR)\xsc.obj \
	$(LIB_DIR)\xslBase.obj

MODULES = \
	$(MOD_DIR)\commodetto\Bitmap.xsb \
	$(MOD_DIR)\commodetto\BMPOut.xsb \
	$(MOD_DIR)\commodetto\BufferOut.xsb \
	$(MOD_DIR)\commodetto\ColorCellOut.xsb \
	$(MOD_DIR)\commodetto\Convert.xsb \
	$(MOD_DIR)\commodetto\ParseBMF.xsb \
	$(MOD_DIR)\commodetto\ParseBMP.xsb \
	$(MOD_DIR)\commodetto\PixelsOut.xsb \
	$(MOD_DIR)\commodetto\Poco.xsb \
	$(MOD_DIR)\commodetto\PocoCore.xsb \
	$(MOD_DIR)\commodetto\ReadJPEG.xsb \
	$(MOD_DIR)\commodetto\ReadPNG.xsb \
	$(MOD_DIR)\commodetto\RLE4Out.xsb \
	$(MOD_DIR)\wavreader.xsb \
	$(MOD_DIR)\base64.xsb \
	$(MOD_DIR)\ber.xsb \
	$(MOD_DIR)\file.xsb \
	$(MOD_DIR)\buildclut.xsb \
	$(MOD_DIR)\cdv.xsb \
	$(MOD_DIR)\colorcellencode.xsb \
	$(MOD_DIR)\compileDataView.xsb \
	$(MOD_DIR)\compressbmf.xsb \
	$(MOD_DIR)\image2cs.xsb \
	$(MOD_DIR)\mcbundle.xsb \
	$(MOD_DIR)\mcconfig.xsb \
	$(MOD_DIR)\mclocal.xsb \
	$(MOD_DIR)\mcmanifest.xsb \
	$(MOD_DIR)\mcpack.xsb \
	$(MOD_DIR)\mcrez.xsb \
	$(MOD_DIR)\nodered2mcu.xsb \
	$(MOD_DIR)\png2bmp.xsb \
	$(MOD_DIR)\resampler.xsb \
	$(MOD_DIR)\rle4encode.xsb \
	$(MOD_DIR)\transform.xsb \
	$(MOD_DIR)\tool.xsb \
	$(MOD_DIR)\unicode-ranges.xsb \
	$(MOD_DIR)\wav2maud.xsb \
	$(MOD_DIR)\bles2gatt.xsb \
	$(MOD_DIR)\url.xsb \
	$(TMP_DIR)\modBase64.xsi \
	$(TMP_DIR)\commodettoBitmap.xsi \
	$(TMP_DIR)\commodettoBufferOut.xsi \
	$(TMP_DIR)\commodettoColorCellOut.xsi \
	$(TMP_DIR)\commodettoConvert.xsi \
	$(TMP_DIR)\commodettoPocoCore.xsi \
	$(TMP_DIR)\commodettoPocoBlit.xsi \
	$(TMP_DIR)\commodettoParseBMP.xsi \
	$(TMP_DIR)\commodettoParseBMF.xsi \
	$(TMP_DIR)\commodettoReadJPEG.xsi \
	$(TMP_DIR)\commodettoReadPNG.xsi \
	$(TMP_DIR)\image2cs.xsi \
	$(TMP_DIR)\miniz.xsi \
	$(TMP_DIR)\modInstrumentation.xsi \
	$(TMP_DIR)\tool.xsi \
	$(TMP_DIR)\url.xsi
PRELOADS =\
	-p commodetto\Bitmap.xsb\
	-p commodetto\BMPOut.xsb\
	-p commodetto\BufferOut.xsb\
	-p commodetto\ColorCellOut.xsb\
	-p commodetto\Convert.xsb\
	-p commodetto\ParseBMF.xsb\
	-p commodetto\ParseBMP.xsb\
	-p commodetto\Poco.xsb\
	-p commodetto\PocoCore.xsb\
	-p commodetto\ReadPNG.xsb\
	-p commodetto\RLE4Out.xsb\
	-p wavreader.xsb\
	-p base64.xsb\
	-p ber.xsb\
	-p resampler.xsb\
	-p transform.xsb\
	-p unicode-ranges.xsb\
	-p file.xsb\
	-p url.xsb
CREATION = -c 134217728,16777216,8388608,1048576,16384,16384,0,1993,127,32768,1993,0,main

HEADERS =\
	$(COMMODETTO)\commodettoBitmap.h\
	$(COMMODETTO)\commodettoPocoBlit.h\
	$(INSTRUMENTATION)\modInstrumentation.h
OBJECTS = \
	$(TMP_DIR)\adpcm-lib.obj \
	$(TMP_DIR)\commodettoBitmap.obj \
	$(TMP_DIR)\commodettoBufferOut.obj \
	$(TMP_DIR)\commodettoColorCellOut.obj \
	$(TMP_DIR)\commodettoConvert.obj \
	$(TMP_DIR)\commodettoParseBMP.obj \
	$(TMP_DIR)\commodettoParseBMF.obj \
	$(TMP_DIR)\commodettoPocoCore.obj \
	$(TMP_DIR)\commodettoPocoBlit.obj \
	$(TMP_DIR)\commodettoReadJPEG.obj \
	$(TMP_DIR)\commodettoReadPNG.obj \
	$(TMP_DIR)\cfeBMF.obj \
	$(TMP_DIR)\image2cs.obj \
	$(TMP_DIR)\mcpack.obj \
	$(TMP_DIR)\miniz.obj \
	$(TMP_DIR)\modBase64.obj \
	$(TMP_DIR)\modInstrumentation.obj \
	$(TMP_DIR)\tool.obj \
	$(TMP_DIR)\wav2maud.obj \
	$(TMP_DIR)\url.obj

COMMANDS = \
	$(BIN_DIR)\buildclut.bat \
	$(BIN_DIR)\cdv.bat \
	$(BIN_DIR)\colorcellencode.bat \
	$(BIN_DIR)\compressbmf.bat \
	$(BIN_DIR)\image2cs.bat \
	$(BIN_DIR)\mcbundle.bat \
	$(BIN_DIR)\mcconfig.bat \
	$(BIN_DIR)\mclocal.bat \
	$(BIN_DIR)\mcpack.bat \
	$(BIN_DIR)\mcrez.bat \
	$(BIN_DIR)\nodered2mcu.bat \
	$(BIN_DIR)\png2bmp.bat \
	$(BIN_DIR)\rle4encode.bat \
	$(BIN_DIR)\wav2maud.bat \
	$(BIN_DIR)\bles2gatt.bat

!IF EXISTS($(TOOLS)\mcrun.js)
COMMANDS = $(COMMANDS) $(BIN_DIR)\mcrun.bat
MODULES = $(MODULES) $(MOD_DIR)\mcrun.xsb
!ENDIF

C_OPTIONS = \
	/c \
	/D _CONSOLE \
	/D WIN32 \
	/D _CRT_SECURE_NO_DEPRECATE \
	/D HAVE_MEMMOVE=1 \
	/D INCLUDE_XSPLATFORM=1 \
	/D XSPLATFORM=\"win_xs.h\" \
	/D XSTOOLS=1 \
	/D mxRun=1 \
	/D mxParse=1 \
	/D mxNoFunctionLength=1 \
	/D mxNoFunctionName=1 \
	/D mxHostFunctionPrimitive=1 \
	/D mxFewGlobalsTable=1 \
	/D mxMessageWindowClass=\"fxMessageWindowClassX\" \
	/D kModdableToolsVersion=\"$(TOOLS_VERSION)\" \
	/I$(XS_DIR)\includes \
	/I$(XS_DIR)\platforms \
	/I$(XS_DIR)\sources \
	/I$(COMMODETTO) \
	/I$(INSTRUMENTATION) \
	/I$(TOOLS) \
	/I$(TMP_DIR) \
	/nologo \
	/MP
	
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
	
LINK_OPTIONS = /incremental:no /nologo /subsystem:console
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

$(BIN_DIR)\tools.exe : $(XS_OBJECTS) $(TMP_DIR)\mc.xs.obj $(OBJECTS)
	link \
		$(LINK_OPTIONS) \
		$(LIBRARIES) \
		$(XS_OBJECTS) \
		$(TMP_DIR)\mc.xs.obj \
		$(OBJECTS) \
		/implib:$(TMP_DIR)\$(NAME).lib \
		/out:$(BIN_DIR)\tools.exe

$(XS_OBJECTS) : $(XS_HEADERS)
{$(XS_DIR)\platforms\}.c{$(LIB_DIR)\}.obj::
	cd $(LIB_DIR)
	cl $< $(C_OPTIONS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.obj::
	cd $(LIB_DIR)
	cl $< $(C_OPTIONS)
{$(XS_DIR)\tools\}.c{$(LIB_DIR)\}.obj::
	cd $(LIB_DIR)
	cl $< $(C_OPTIONS)

$(TMP_DIR)\mc.xs.obj: $(TMP_DIR)\mc.xs.c $(HEADERS)
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
$(MOD_DIR)\commodetto\BufferOut.xsb : $(COMMODETTO)\commodettoBufferOut.js
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
$(MOD_DIR)\commodetto\PocoCore.xsb : $(COMMODETTO)\commodettoPocoCore.js
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
$(MOD_DIR)\url.xsb : $(DATA)\url\url.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR) -r $(@B)
$(MOD_DIR)\wavreader.xsb : $(DATA)\wavreader\wavreader.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR) -r $(@B)
$(MOD_DIR)\base64.xsb : $(DATA)\base64\base64.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR) -r $(@B)
$(MOD_DIR)\ber.xsb : $(CRYPT)\etc\ber.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR) -r $(@B)
$(MOD_DIR)\transform.xsb : $(CRYPT)\etc\transform.js
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $** -c -d -e -o $(MOD_DIR) -r $(@B)
{$(TOOLS)\}.js{$(MOD_DIR)\}.xsb:
	@echo # xsc $(**F)
	$(BIN_DIR)\xsc $< -c -d -e -o $(MOD_DIR)
	
{$(COMMODETTO)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(@F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR) -r $(@F)
{$(INSTRUMENTATION)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(@F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR) -r $(@F)
{$(TOOLS)\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(@F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR) -r $(@F)
{$(DATA)\url\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(@F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR) -r $(@F)
{$(DATA)\base64\}.c{$(TMP_DIR)\}.xsi:
	@echo # xsid $(@F)
	$(BIN_DIR)\xsid $< -o $(TMP_DIR) -r $(@F)

$(TMP_DIR)\tool.obj : $(MODDABLE)\tools\VERSION
$(OBJECTS) : $(XS_HEADERS) $(HEADERS)
{$(COMMODETTO)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(INSTRUMENTATION)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(TOOLS)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(DATA)\url\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(DATA)\base64\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)
{$(TMP_DIR)\}.c{$(TMP_DIR)\}.obj::
	cd $(TMP_DIR)
	cl $< $(C_OPTIONS)

$(BIN_DIR)\buildclut.bat :
	@echo # buildclut.bat
	echo @%~dp0\tools buildclut %%* 1> $(BIN_DIR)\buildclut.bat
$(BIN_DIR)\cdv.bat :
	@echo # cdv.bat
	echo @%~dp0\tools cdv %%* 1> $(BIN_DIR)\cdv.bat
$(BIN_DIR)\colorcellencode.bat :
	@echo # colorcellencode.bat
	echo @%~dp0\tools colorcellencode %%* 1> $(BIN_DIR)\colorcellencode.bat
$(BIN_DIR)\compressbmf.bat :
	@echo # compressbmf.bat
	echo @%~dp0\tools compressbmf %%* 1> $(BIN_DIR)\compressbmf.bat
$(BIN_DIR)\image2cs.bat :
	@echo # image2cs.bat
	echo @%~dp0\tools image2cs %%* 1> $(BIN_DIR)\image2cs.bat
$(BIN_DIR)\mcbundle.bat :
	@echo # mcbundle.bat
	echo @%~dp0\tools mcbundle %%* 1> $(BIN_DIR)\mcbundle.bat	
$(BIN_DIR)\mcconfig.bat :
	@echo # mcconfig.bat
	echo @%~dp0\tools mcconfig %%* 1> $(BIN_DIR)\mcconfig.bat
$(BIN_DIR)\mclocal.bat :
	@echo # mclocal.bat
	echo @%~dp0\tools mclocal %%* 1> $(BIN_DIR)\mclocal.bat
$(BIN_DIR)\mcpack.bat :
	@echo # mcpack.bat
	echo @%~dp0\tools mcpack %%* 1> $(BIN_DIR)\mcpack.bat
$(BIN_DIR)\mcrez.bat :
	@echo # mcrez.bat
	echo @%~dp0\tools mcrez %%* 1> $(BIN_DIR)\mcrez.bat
$(BIN_DIR)\mcrun.bat :
	@echo # mcrun.bat
	echo @%~dp0\tools mcrun %%* 1> $(BIN_DIR)\mcrun.bat
$(BIN_DIR)\nodered2mcu.bat :
	@echo # nodered2mcu.bat
	echo @%~dp0\tools nodered2mcu %%* 1> $(BIN_DIR)\nodered2mcu.bat
$(BIN_DIR)\png2bmp.bat :
	@echo # png2bmp.bat
	echo @%~dp0\tools png2bmp %%* 1> $(BIN_DIR)\png2bmp.bat
$(BIN_DIR)\rle4encode.bat :
	@echo # rle4encode.bat
	echo @%~dp0\tools rle4encode %%* 1> $(BIN_DIR)\rle4encode.bat
$(BIN_DIR)\wav2maud.bat :
	@echo # wav2maud.bat
	echo @%~dp0\tools wav2maud %%* 1> $(BIN_DIR)\wav2maud.bat
$(BIN_DIR)\bles2gatt.bat :
	@echo # bles2gatt.bat
	echo @%~dp0\tools bles2gatt %%* 1> $(BIN_DIR)\bles2gatt.bat

clean :
	del /Q $(BUILD_DIR)\bin\win\debug\$(NAME).*
	del /Q $(BUILD_DIR)\bin\win\release\$(NAME).*
	del /Q $(BUILD_DIR)\tmp\win\debug\$(NAME)
	del /Q $(BUILD_DIR)\tmp\win\release\$(NAME)

