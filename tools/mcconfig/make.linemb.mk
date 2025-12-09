# ==================== START OF make.linemb.mk ====================
all: build

ifeq ("$(SUBPLATFORM)","arm64") # ARM64 toolchain
ARCH_PREFIX = aarch64-linux-gnu-
endif

ifeq ("$(SUBPLATFORM)","armhf") # ARM toolchain
ARCH_PREFIX = arm-linux-gnueabihf-
endif

ifeq ("$(SUBPLATFORM)","rv1106") # ARM toolchain
ARCH_PREFIX = arm-rockchip830-linux-uclibcgnueabihf-
endif

ifeq ("$(SUBPLATFORM)","x86_64") # x86 toolchain
ARCH_PREFIX = ""
endif

ifndef ARCH_PREFIX # check if the toolchain is set
$(error SUBPLATFORM is not set. Please set SUBPLATFORM to arm, arm64 or x86)
endif

ifeq ("$(SUBPLATFORM)","rv1106") # ARM toolchain
CC = $(ARCH_PREFIX)gcc
CXX = $(ARCH_PREFIX)g++
LD = $(ARCH_PREFIX)gcc
else 
CC = /usr/bin/$(ARCH_PREFIX)gcc
CXX = /usr/bin/$(ARCH_PREFIX)g++
endif

# -DINCLUDE_XSPLATFORM=1 \
# -DXSPLATFORM=\"linarm_xs.h\"

C_DEFINES = \
	-DINCLUDE_XSPLATFORM=1 \
	-DXSPLATFORM=\"lin_xs.h\" \
	-DXS_ARCHIVE=1 \
	-DmxRun=1 \
	-DmxNoFunctionLength=1 \
	-DmxNoFunctionName=1 \
	-DmxHostFunctionPrimitive=1 \
	-DmxFewGlobalsTable=1 \
	-DkCommodettoBitmapFormat=$(COMMODETTOBITMAPFORMAT) \
	-DkPocoRotation=$(POCOROTATION)

C_DEFINES += \
	-Wno-misleading-indentation \
	-Wno-implicit-fallthrough

PKGCONFIG = $(shell which $(ARCH_PREFIX)pkg-config)

ifeq ("$(SUBPLATFORM)","rv1106") # ARM toolchain
	INCLUDE_PATH = /opt/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/include
	C_FLAGS = -fPIC -c -I$(INCLUDE_PATH)/libmount -I$(INCLUDE_PATH)/blkid -I$(INCLUDE_PATH)/glib-2.0 
	LINK_LIBRARIES = -lgio-2.0 -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lpthread -lm -lc -ldl -lffi
else
	C_FLAGS = -fPIC -c $(shell $(PKGCONFIG) --cflags glib-2.0 gio-2.0 gmodule-2.0)
	LINK_LIBRARIES = $(shell $(PKGCONFIG) --libs glib-2.0 gio-2.0 gmodule-2.0) -lpthread -lm -lc -ldl -latomic -lresolv -lz -lmount -lblkid -lffi -lselinux -lpcre
endif

ifeq ("$(SUBPLATFORM)","armhf") # ARM toolchain
	LINK_OPTIONS += -static
endif

# DEBUG and INSTRUMENT
ifeq ($(DEBUG),)
	C_FLAGS += -D_RELEASE=1 -O3
else
	C_FLAGS += -D_DEBUG=1 -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
#	C_FLAGS += -DMC_MEMORY_DEBUG=1
endif

ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif

#
# Include the XS engine
#
XS_DIRECTORIES = \
	$(XS_DIR)/includes \
	$(XS_DIR)/platforms \
	$(XS_DIR)/sources

XS_HEADERS = \
	$(XS_DIR)/platforms/lin_xs.h \
	$(XS_DIR)/platforms/mc/xsHosts.h \
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
	$(LIB_DIR)/xsre.c.o \

MODULE_DIRS = \
	$(MODDABLE)/modules/base/timer\
	$(MODDABLE)/modules/base/instrumentation

C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(XS_DIRECTORIES) $(TMP_DIR) $(MODULE_DIRS),-I$(dir))

# XS related targets
VPATH += $(XS_DIRECTORIES)

$(XS_OBJECTS) : $(XS_HEADERS)

$(LIB_DIR)/%.c.o: %.c
	@echo "# cc" $(<F)
	@$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c.o: $(TMP_DIR)/mc.xs.c $(XS_HEADERS)
	@echo "# cc" $(<F)
	@$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	@xsl -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.o: $(TMP_DIR)/mc.resources.c
	@echo "# cc" $(<F)
	@$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -c -o $@

$(TMP_DIR)/mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST) $(SDKCONFIG_H) 
	@echo "# mcrez resources"
	@mcrez $(DATA) $(RESOURCES) -o $(TMP_DIR) -r mc.resources.c

# The exectuable
$(TMP_DIR)/xs_main.o: $(BUILD_DIR)/devices/linemb/xsProj-glib/main.c
	@echo "# cc" $(<F)
	@$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -c $< -o $@

$(BIN_DIR)/$(NAME): ${XS_OBJECTS} $(TMP_DIR)/mc.xs.c.o $(OBJECTS) $(TMP_DIR)/xs_main.o $(TMP_DIR)/mc.resources.o
	@echo "# ld " $@
	@$(CC) $(LINK_OPTIONS) $^ $(LINK_LIBRARIES) -o $@

build: $(PROJ_DIR) $(BIN_DIR)/$(NAME)

clean:
	echo "# Clean project"
	-rm -rf $(BIN_DIR) 2>/dev/null
	-rm -rf $(TMP_DIR) 2>/dev/null
	-rm -rf $(LIB_DIR) 2>/dev/null

# ==================== END OF make.linemb.mk ====================