#
# NEEDS BOILERPLATE
#     Copyright (C) 2016-2017 Moddable Tech, Inc.
#     All rights reserved.
#

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/ios/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/ios/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/ios/release/lib
	endif
endif

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources \
	$(XS_DIR)/sources/pcre

XS_HEADERS = \
	$(XS_DIR)/platforms/ios_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)/ios_xs.c.o \
	$(LIB_DIR)/xsAll.c.o \
	$(LIB_DIR)/xsAPI.c.o \
	$(LIB_DIR)/xsArray.c.o \
	$(LIB_DIR)/xsAtomics.c.o \
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
	$(LIB_DIR)/xspcre.c.o

HEADERS += $(XS_HEADERS)

SDKROOT = "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
TARGET = -arch armv7 -arch arm64 -isysroot $(SDKROOT) -miphoneos-version-min=6.0

C_DEFINES = \
	-DXS_ARCHIVE=1 \
	-DINCLUDE_XSPLATFORM=1 \
	-DXSPLATFORM=\"ios_xs.h\" \
	-DmxRun=1 \
	-DmxParse=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif
C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(TMP_DIR),-I$(dir))
XS_C_FLAGS = -c $(TARGET)
ifeq ($(DEBUG),)
	XS_C_FLAGS += -D_RELEASE=1 -O3
else
	XS_C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif
C_FLAGS = $(XS_C_FLAGS) 
 
LIBRARIES = -framework Foundation -framework UIKit -framework CoreGraphics -framework CoreImage -framework CoreText

LINK_FLAGS = $(TARGET)

XSC = $(BUILD_DIR)/bin/mac/debug/xsc
XSID = $(BUILD_DIR)/bin/mac/debug/xsid
XSL = $(BUILD_DIR)/bin/mac/debug/xsl
	
VPATH += $(XS_DIRECTORIES)

.PHONY: all	
	
all: $(LIB_DIR) $(BIN_DIR)/Info.plist $(BIN_DIR)/main $(RESOURCES)
	
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(BIN_DIR)/Info.plist: $(MAIN_DIR)/Info.plist
	@echo "# cp Info.plist"
	cp -rf $< $@
	
$(BIN_DIR)/main: $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(OBJECTS)
	@echo "# cc" $(@F)
	$(CC) $(LINK_FLAGS) $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(OBJECTS) $(LIBRARIES) -o $@
	dsymutil $(BIN_DIR)/main -o $(BIN_DIR).dSYM

$(XS_OBJECTS) : $(XS_HEADERS)
$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(XS_C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c.o: $(TMP_DIR)/mc.xs.c $(HEADERS)
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@
	
$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -t -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(CREATION) $(MODULES)
	
MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
