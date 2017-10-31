#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\synergy\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\synergy\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\synergy\release\lib
!ENDIF

CROSS_BASE = "C:\Program Files\GNU Tools ARM Embedded\4.9 2015q3"
CROSS_INC = \
	-I$(CROSS_BASE)\arm-none-eabi\include \
	-I$(CROSS_BASE)\lib\gcc\arm-none-eabi\4.9.3\include \
	-I$(CROSS_BASE)\lib\gcc\arm-none-eabi\4.9.3\include-fixed

CROSS_COMPILE = arm-none-eabi
CC = $(CROSS_COMPILE)-gcc
AR = $(CROSS_COMPILE)-ar

AR_OPTIONS = rcs

XS_DIRECTORIES = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\platforms\synergy \
	-I$(XS_DIR)\sources\pcre

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\sources\xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)\xsdtoa.o \
	$(LIB_DIR)\xspcre.o \
	$(LIB_DIR)\xsCommon.o \
	$(LIB_DIR)\xsAll.o \
	$(LIB_DIR)\xsDebug.o \
	$(LIB_DIR)\xsMemory.o \
	$(LIB_DIR)\xsSymbol.o \
	$(LIB_DIR)\xsType.o \
	$(LIB_DIR)\xsProperty.o \
	$(LIB_DIR)\xsGlobal.o \
	$(LIB_DIR)\xsObject.o \
	$(LIB_DIR)\xsFunction.o \
	$(LIB_DIR)\xsArray.o \
	$(LIB_DIR)\xsAtomics.o \
	$(LIB_DIR)\xsString.o \
	$(LIB_DIR)\xsBoolean.o \
	$(LIB_DIR)\xsNumber.o \
	$(LIB_DIR)\xsMath.o \
	$(LIB_DIR)\xsDate.o \
	$(LIB_DIR)\xsError.o \
	$(LIB_DIR)\xsJSON.o \
	$(LIB_DIR)\xsDataView.o \
	$(LIB_DIR)\xsAPI.o \
	$(LIB_DIR)\xsRun.o \
	$(LIB_DIR)\xsGenerator.o \
	$(LIB_DIR)\xsModule.o \
	$(LIB_DIR)\xsProfile.o \
	$(LIB_DIR)\xsPromise.o \
	$(LIB_DIR)\xsProxy.o \
	$(LIB_DIR)\xsMapSet.o \
	$(LIB_DIR)\xsMarshall.o \
	$(LIB_DIR)\xsRegExp.o \
	$(LIB_DIR)\xsScript.o \
	$(LIB_DIR)\xsLexical.o \
	$(LIB_DIR)\xsSyntaxical.o \
	$(LIB_DIR)\xsTree.o \
	$(LIB_DIR)\xsSourceMap.o \
	$(LIB_DIR)\xsScope.o \
	$(LIB_DIR)\xsCode.o \
	$(LIB_DIR)\xsmc.o \
	$(LIB_DIR)\xsPlatform.o

DIRECTORIES = $(DIRECTORIES) $(XS_DIRECTORIES)
	
HEADERS = $(HEADERS) $(XS_HEADERS)

#	-DXS_ARCHIVE=1
#	-DmxMC=1
#	-DmxParse=1
#	-DmxNoFunctionLength=1
#	-DmxNoFunctionName=1
#	-DmxHostFunctionPrimitive=1
#	-DmxFewGlobalsTable=1

C_DEFINES = \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-U__STRICT_ANSI__ \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoFrameBuffer=1 \
	-DkPocoCLUT16_01=0 \
	-DkPocoRotation=$(ROTATION)
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) \
	-DMODINSTRUMENTATION=1
!ENDIF

C_INCLUDES = $(DIRECTORIES) -I$(TMP_DIR)

C_FLAGS = $(C_FLAGS) \
	-std=c99 \
	-c \
	-mcpu=cortex-m4 \
	-mthumb \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-fmessage-length=0 \
	-fsigned-char \
	-ffunction-sections \
	-fdata-sections \
	-D_RENESAS_SYNERGY_ 
#	-DSTATIC_XS_HEAP_SIZE=222880
!IF "$(DEBUG)"=="1"
C_FLAGS = $(C_FLAGS) \
	-DmxDebug=1 \
	-D_DEBUG \
	-O0 \
	-g3 \
	-Wall \
	-Wextra \
	-Wno-missing-field-initializers \
	-Wno-unused-parameter
!ELSE
C_FLAGS = $(C_FLAGS) \
	-O2
!ENDIF

BUILDCLUT = $(BUILD_DIR)\bin\win\debug\buildclut
COMPRESSBMF = $(BUILD_DIR)\bin\win\debug\compressbmf
RLE4ENCODE = $(BUILD_DIR)\bin\win\debug\rle4encode
MCLOCAL = $(BUILD_DIR)\bin\win\debug\mclocal
MCREZ = $(BUILD_DIR)\bin\win\debug\mcrez
PNG2BMP = $(BUILD_DIR)\bin\win\debug\png2bmp
IMAGE2CS = $(BUILD_DIR)\bin\win\debug\image2cs
XSC = $(BUILD_DIR)\bin\win\debug\xsc
XSID = $(BUILD_DIR)\bin\win\debug\xsid
XSL = $(BUILD_DIR)\bin\win\debug\xsl

ARCHIVE_FILE = $(BIN_DIR)\xsr.ar

all: $(LIB_DIR) $(ARCHIVE_FILE)

$(LIB_DIR) :
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

delAr:
	@del $(ARCHIVE_FILE)

$(ARCHIVE_FILE): $(XS_OBJECTS) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsPlatform.o
	@echo # archive $(ARCHIVE_FILE)
	$(AR) $(AR_OPTIONS) $@ $(TMP_DIR)\mc.xs.o $(TMP_DIR)\xsHost.o $(TMP_DIR)\xsPlatform.o $(TMP_DIR)\mc.resources.o
	
$(XS_OBJECTS) : $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # $(CC) $(<F)
	@echo # $(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	$(AR) $(AR_OPTIONS) $(ARCHIVE_FILE) $@

#	-I$(XS_DIR)\platforms\synergy \

$(TMP_DIR)\xsHost.o: $(XS_DIR)\platforms\synergy\xsHost.c
	@echo # $(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\xsPlatform.o: $(XS_DIR)\platforms\synergy\xsPlatform.c
	@echo # $(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	@echo # $(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	@echo # $(CC) $?
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $? -o $@

$(TMP_DIR)\mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -r mc.resources.c

