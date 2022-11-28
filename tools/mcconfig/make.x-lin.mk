#
# Copyright (c) 2016-2021  Moddable Tech, Inc.
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

XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources

XS_HEADERS = \
	$(XS_DIR)/platforms/lin_xs.h \
	$(XS_DIR)/platforms/xsPlatform.h \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsScript.h

XS_OBJECTS = \
	$(LIB_DIR)/lin_xs.c.o \
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
	$(LIB_DIR)/xsre.c.o

HEADERS += $(XS_HEADERS)

PKGCONFIG = $(shell which pkg-config)
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

C_DEFINES = \
	-DXS_ARCHIVE=1 \
	-DINCLUDE_XSPLATFORM=1 \
	-DXSPLATFORM=\"lin_xs.h\" \
	-DmxRun=1 \
	-DmxParse=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1
C_DEFINES += \
	-Wno-misleading-indentation \
	-Wno-implicit-fallthrough
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif
C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(TMP_DIR),-I$(dir))
C_FLAGS = -fPIC -c $(shell $(PKGCONFIG) --cflags freetype2 gtk+-3.0)
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

LINK_LIBRARIES = -lm -lc $(shell $(PKGCONFIG) --libs freetype2 gtk+-3.0) -ldl -latomic -lpthread

LINK_OPTIONS = -fPIC

ifeq ($(DEBUG),1)
MCLOCAL = $(BUILD_DIR)/bin/mac/debug/mclocal
MCREZ = $(BUILD_DIR)/bin/lin/debug/mcrez
XSC = $(BUILD_DIR)/bin/lin/debug/xsc
XSID = $(BUILD_DIR)/bin/lin/debug/xsid
XSL = $(BUILD_DIR)/bin/lin/debug/xsl
else
MCLOCAL = $(BUILD_DIR)/bin/mac/release/mclocal
MCREZ = $(BUILD_DIR)/bin/lin/release/mcrez
XSC = $(BUILD_DIR)/bin/lin/release/xsc
XSID = $(BUILD_DIR)/bin/lin/release/xsid
XSL = $(BUILD_DIR)/bin/lin/release/xsl
endif

VPATH += $(XS_DIRECTORIES)

.PHONY: all	

all: build

build: $(LIB_DIR) $(BIN_DIR)/$(NAME)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(BIN_DIR)/$(NAME): $(XS_OBJECTS) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.o $(OBJECTS)
	@echo "# ld " $(<F)
	$(CC) $(LINK_OPTIONS) $^ $(LINK_LIBRARIES) -o $@

clean:
	@echo "# Clean project"
	-rm -rf $(BIN_DIR) 2>/dev/null
	-rm -rf $(TMP_DIR) 2>/dev/null

$(XS_OBJECTS) : $(XS_HEADERS)
$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c.o: $(TMP_DIR)/mc.xs.c $(HEADERS)
	@echo "# cc" $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.o: $(RESOURCES_DIR)/mc.resources.c
	@echo "# cc" $(<F)
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -c -o $@

$(RESOURCES_DIR)/mc.resources.c: $(RESOURCES_DIR)/mc.resources.xml
	@echo "# glib-compile-resources" $(<F)
	cd $(RESOURCES_DIR); $(GLIB_COMPILE_RESOURCES) --generate-source --c-name mc mc.resources.xml

$(RESOURCES_DIR)/mc.resources.xml: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(RESOURCES_DIR) -p x-lin -r mc.resources.xml -s $(SLASH_SIGNATURE)

INSTALL_BIN_DIR = /usr/bin
INSTALL_DESKTOP_DIR = /usr/share/applications/
INSTALL_ICON_DIR = /usr/share/icons/hicolor
INSTALL_ICONS = $(foreach SIZE,32 48 64 96 128 256,$(INSTALL_ICON_DIR)/$(SIZE)x$(SIZE)/apps/$(DASH_SIGNATURE).png)

install: $(INSTALL_BIN_DIR)/$(DASH_SIGNATURE) $(INSTALL_DESKTOP_DIR)/$(DASH_SIGNATURE).desktop $(INSTALL_ICONS)
	sudo gtk-update-icon-cache -f $(INSTALL_ICON_DIR)

$(INSTALL_BIN_DIR)/$(DASH_SIGNATURE): $(BIN_DIR)/$(NAME)
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@

$(INSTALL_DESKTOP_DIR)/$(DASH_SIGNATURE).desktop: $(MAIN_DIR)/lin/main.desktop
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@

$(INSTALL_ICON_DIR)/%/apps/$(DASH_SIGNATURE).png: $(MAIN_DIR)/lin/icons/%.png
	@echo "#" $(NAME) $(GOAL) ": cp" $(<F)
	sudo cp $< $@


# MAKEFLAGS += --jobs
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif
