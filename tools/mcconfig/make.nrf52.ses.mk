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

HOST_OS := $(shell uname)

PLATFORM_DIR = $(MODDABLE)/build/devices/nrf52

ifeq ($(HOST_OS),Darwin)
	NRF52_GCC_ROOT ?= /Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 4.22/gcc
	SEGGER_INCLUDE = "/Applications/SEGGER\ Embedded\ Studio\ for\ ARM\ 4.22/include"
else
	NRF52_GCC_ROOT ?= /usr/share/segger_embedded_studio_for_arm_4.30c/gcc
	SEGGER_INCLUDE = /usr/share/segger_embedded_studio_for_arm_4.30c/include
endif

NRF_SDK_DIR = $(HOME)/nRF5/nRF5_SDK
NRFJPROG = $(HOME)/nRF5/nrfjprog/nrfjprog

# nRF52840_xxAA
BOARD = pca10056
SOFT_DEVICE = s140
SDK_ROOT = $(NRF_SDK_DIR)
HWCPU = cortex-m4

HW_DEBUG_OPT = $(FP_OPTS) # -flto
HW_OPT = -O2 $(FP_OPTS) # -flto
#DEV_C_FLAGS = -Dnrf52

# changed from default NRF:
# FP_OPTS
# remove  -fshort-enums, use -fno-short-enums

ifeq ($(DEBUG),1)
	LIB_DIR = $(BUILD_DIR)/tmp/nrf52/debug/lib
else
	ifeq ($(INSTRUMENT),1)
		LIB_DIR = $(BUILD_DIR)/tmp/nrf52/instrument/lib
	else
		LIB_DIR = $(BUILD_DIR)/tmp/nrf52/release/lib
	endif
endif


# C flags common to all targets
CFLAGS += -DBOARD_PCA10056
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52840_XXAA
CFLAGS += -DSWI_DISABLE0
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -munaligned-access
CFLAGS += -Wall  # -Werror
# CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fno-short-enums
CFLAGS += -DFREERTOS
CFLAGS += -DNRF_SD_BLE_API_VERSION=6
CFLAGS += -DS140
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DNRF_DRV_UART_WITH_UARTE

# Assembler flags common to all targets
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
# ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBOARD_PCA10056
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52840_XXAA
ASMFLAGS += $(FP_OPTS)

ASMFLAGS += -DFREERTOS
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=6
ASMFLAGS += -DS140
ASMFLAGS += -DSOFTDEVICE_PRESENT

# Linker flags
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
# LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += $(FP_OPTS)
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
# LDFLAGS += --specs=nano.specs

LIB_FILES += -lc -lnosys -lm

# nrf52840_xxaa: CFLAGS += -D__HEAP_SIZE=8192
# nrf52840_xxaa: CFLAGS += -D__STACK_SIZE=8192
# nrf52840_xxaa: ASMFLAGS += -D__HEAP_SIZE=8192
# nrf52840_xxaa: ASMFLAGS += -D__STACK_SIZE=8192

INC_DIRS = \
	$(XS_DIR)/../modules/base/instrumentation \
	$(XS_DIR)/../modules/base/timer \
	$(BUILD_DIR)/devices/nrf52 \

SDK_SRC=\

# Include folders common to all targets
INC_DIRS += \
  $(PLATFORM_DIR) \
  $(PLATFORM_DIR)/config \
  $(SDK_ROOT)/external/freertos/source/include \
  $(SDK_ROOT)/external/freertos/portable/GCC/nrf52 \
  $(SDK_ROOT)/external/freertos/portable/CMSIS/nrf52 \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/modules/nrfx/mdk \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/libraries/serial \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/components/libraries/hardfault/nrf52 \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/spi_mngr \
  $(SDK_ROOT)/components/libraries/twi_sensor \
  $(SDK_ROOT)/components/libraries/twi_mngr \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/integration/nrfx/legacy \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/softdevice/$(SOFT_DEVICE)/headers/nrf52 \
  $(SDK_ROOT)/components/softdevice/$(SOFT_DEVICE)/headers \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/ble/nrf_ble_scan \
  $(SDK_ROOT)/components/ble/peer_manager

#  $(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd \

xXS_OBJ = \
	$(LIB_DIR)/xsHost.c.o \
	$(LIB_DIR)/xsPlatform.c.o \
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

XS_DIRS = \
	$(XS_DIR)/includes \
	$(XS_DIR)/sources \
	$(XS_DIR)/platforms/nrf52 \
	$(BUILD_DIR)/devices/nrf52
XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/nrf52/xsPlatform.h \
	$(XS_DIR)/platforms/nrf52/xsHost.h \
	$(BUILD_DIR)/devices/nrf52/config/sdk_config.h

HEADERS += $(XS_HEADERS)

# SDK_GLUE_OBJ in the IDE
SDK_GLUE_OBJ = \

SDK_GLUE_DIRS = \
	$(BUILD_DIR)/devices/nrf52/base \

xx =\
	$(SDK_ROOT)/components/libraries/queue \
	$(SDK_ROOT)/components/libraries/serial \
	$(SDK_ROOT)/components/iot/errno \
	
C_INCLUDES += \
	"-isystem$(SEGGER_INCLUDE)" \

#	"-isystem$(SEGGER_PKG_INCLUDE)"

# TOOLS_BIN = $(NRF52_GCC_ROOT)/bin
# TOOLS_PREFIX = arm-none-eabi-

# CC  = $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc
# CPP = $(TOOLS_BIN)/$(TOOLS_PREFIX)g++
# LD  = $(TOOLS_BIN)/$(TOOLS_PREFIX)ld
# AR  = $(TOOLS_BIN)/$(TOOLS_PREFIX)ar
# OBJCOPY = $(TOOLS_BIN)/$(TOOLS_PREFIX)objcopy
# SIZE  = $(TOOLS_BIN)/$(TOOLS_PREFIX)size

TOOLS_BIN = $(NRF52_GCC_ROOT)/arm-none-eabi/bin

CC = $(TOOLS_BIN)/cc1
AS = $(TOOLS_BIN)/as
AR = $(TOOLS_BIN)/ar
OBJCOPY = $(TOOLS_BIN)/objcopy

AR_FLAGS = crs

ifeq ($(HOST_OS),Darwin)
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/release
else
MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin/release
endif
BUILDCLUT = $(MODDABLE_TOOLS_DIR)/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)/wav2maud
BLES2GATT = $(MODDABLE_TOOLS_DIR)/bles2gatt
XSC = $(MODDABLE_TOOLS_DIR)/xsc
XSID = $(MODDABLE_TOOLS_DIR)/xsid
XSL = $(MODDABLE_TOOLS_DIR)/xsl

#	-DmxNoConsole=1
#	-c \

NRF_ASM_FLAGS= \
	--traditional-format \
	-mcpu=cortex-m4 \
	-mlittle-endian \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-mthumb

NRF_C_DEFINES= \
	-quiet \
	-D__SIZEOF_WCHAR_T=4 \
	-D__ARM_ARCH_7EM__ \
	-D__SES_ARM \
	-D__ARM_ARCH_FPV4_SP_D16__ \
	-D__HEAP_SIZE__=0x13000 \
	-D__SES_VERSION=41800 \
	-D__GNU_LINKER \
	-DBOARD_PCA10056 \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DFREERTOS \
	-DINCLUDE_vTaskSuspend \
	-DINITIALIZE_USER_SECTIONS \
	-DNO_VTOR_CONFIG  \
	-DNRF52840_XXAA \
	-DNRF_SD_BLE_API_VERSION=6 \
	-DS140 \
	-DSOFTDEVICE_PRESENT \
	-Dnrf52 \
    -fno-short-enums \

C_DEFINES = \
	$(NRF_C_DEFINES) \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(COMMODETTOBITMAPFORMAT) \
	-DkPocoRotation=$(POCOROTATION) \
	-std=gnu99 \
	-fomit-frame-pointer \
	-fno-dwarf2-cfi-asm \
	-fno-builtin \
	-ffunction-sections \
	-fdata-sections \
	-fno-short-enums \
	-fno-common

ifeq ($(DEBUG),1)
	C_DEFINES += \
		-DDEBUG=1 \
		-DmxDebug=1 \
		-DDEBUG_NRF \
		-gdwarf-3 \
		-g3 \
		-gpubnames \
		-Og

	C_FLAGS += $(HW_DEBUG_OPT)
	ASM_FLAGS += $(HW_DEBUG_OPT) -DDEBUG_NRF
else
	C_FLAGS += $(HW_OPT)
	ASM_FLAGS += $(HW_OPT)
endif
ifeq ($(INSTRUMENT),1)
	C_DEFINES += -DMODINSTRUMENTATION=1 -DmxInstrument=1
endif

cr := '\n'
sp :=  
sp += 
qs = $(subst ?,\$(sp),$1)
C_INCLUDES += $(DIRECTORIES)
C_INCLUDES += $(foreach dir,$(INC_DIRS) $(SDK_GLUE_DIRS) $(XS_DIRS) $(LIB_DIR) $(TMP_DIR),-I$(call qs,$(dir)))


# C_INCLUDES += \
# 	"-isystem/$(NRF52_GCC_TOOLCHAIN_DIR)/include"


	
C_FLAGS +=  \
	-fmessage-length=0 \
	-fno-diagnostics-show-caret \
	-mcpu=$(HWCPU) \
	-mlittle-endian \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-mthumb	\
	-mthumb-interwork	\
	-mtp=soft \
	-munaligned-access \
	-nostdinc \

# no: nrf52 doesn't support double precision?
#	-mfpu=fpv4-sp-d16
#	-mfloat-abi=softfp

C_FLAGS_NODATASECTION = $(C_FLAGS)

LINKER_SCRIPT := $(PLATFORM_DIR)/generic_gcc_nrf52.ld

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
SRC_GIT_VERSION = $(call git_description,$(NRF_SDK_DIR)/sources)
ESP_GIT_VERSION = $(call git_description,$(ARDUINO_ROOT))
time_string = $(shell perl -e 'use POSIX qw(strftime); print strftime($(1), localtime());')
BUILD_DATE = $(call time_string,"%Y-%m-%d")
BUILD_TIME = $(call time_string,"%H:%M:%S")
MEM_USAGE = \
  'while (<>) { \
      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
	 }\
	 print "\# Memory usage\n";\
	 print sprintf("\#  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

SDK_OBJ = $(subst .ino,.cpp,$(patsubst %,$(LIB_DIR)/%.o,$(notdir $(SDK_SRC))))
SDK_DIRS = $(sort $(dir $(SDK_SRC)))

VPATH += $(SDK_DIRS) $(SDK_GLUE_DIRS) $(XS_DIRS)

.PHONY: all	
.SUFFIXES:
%.d:
.PRECIOUS: %.d %.o

all: $(BLE) $(TMP_DIR) $(LIB_DIR) $(BIN_DIR)/xs_nrf52.lib
	@echo "# Application files and resources have been built. Use SES to complete build."
	@echo "# - Ensure the following \"Project 'xsproj' Options\" are set in SES"
	@echo "# - \"Linker: Additional Linker Options from File\" should refer to:"
	@echo "# -    $(BIN_DIR)/xs_nrf52.ind"
	@echo "# - \"Preprocessor: User Include Directories\" should contain:"
	@echo "# -    $(TMP_DIR)"

clean:
	@echo "# Cleaning project"
	@echo "#  rm $(BIN_DIR)"
	@rm -rf $(BIN_DIR) 2>/dev/null
	@echo "#  rm $(TMP_DIR)"
	@rm -rf $(TMP_DIR) 2>/dev/null

allclean:
	@echo "# Cleaning all nrf52"
	@echo "#  rm $(MODDABLE)/build/bin/nrf52"
	@rm -rf $(MODDABLE)/build/tmp/nrf52 2>/dev/null
	@echo "#  rm $(MODDABLE)/build/tmp/nrf52"
	@rm -rf $(MODDABLE)/build/tmp/nrf52 2> /dev/null
	

xall: $(TMP_DIR) $(LIB_DIR) $(BIN_DIR)/xs_nrf52.bin
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	@echo Flashing xs_nrf52.hex to device.
	$(NRFJPROG) -f nrf52 --program $(TMP_DIR)/xs_nrf52.hex --sectorerase
	@echo Resetting the device.
	$(NRFJPROG) -f nrf52 --reset

$(TMP_DIR):
	mkdir -p $(TMP_DIR)
	
$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h
	
$(BIN_DIR)/xs_nrf52.bin: $(TMP_DIR)/xs_nrf52.hex
	$(OBJCOPY) -O binary $(TMP_DIR)/xs_nrf52.out $(BIN_DIR)/xs_nrf52.bin

$(TMP_DIR)/xs_nrf52.hex: $(TMP_DIR)/xs_nrf52.out
	@echo "# Size"
	$(SIZE) $(TMP_DIR)/xs_nrf52.out
	$(OBJCOPY) $(TMP_DIR)/xs_nrf52.out $(TMP_DIR)/xs_nrf52.hex

FINAL_LINK_OBJ:=\
	$(SDK_GLUE_OBJ) $(SDK_OBJ) \
	$(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o \
	$(OBJECTS) \
	$(LIB_DIR)/buildinfo.c.o

ekoFiles = $(foreach fil,$(FINAL_LINK_OBJ),$(shell echo '$(strip $(fil))' >> $(BIN_DIR)/xs_nrf52.ind1))

$(BIN_DIR)/xs_nrf52.lib: $(FINAL_LINK_OBJ)
	@echo "# creating xs_nrf52.ind"
	@rm -f $(BIN_DIR)/xs_nrf52.ind 2>null
	$(ekoFiles)
	@mv $(BIN_DIR)/xs_nrf52.ind1 $(BIN_DIR)/xs_nrf52.ind

$(LIB_DIR)/buildinfo.c.o: $(SDK_GLUE_OBJ) $(XS_OBJ) $(SDK_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS)
	@echo "# buildinfo"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.c
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $(LIB_DIR)/buildinfo.c -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@

flash: $(BIN_DIR)/xs_nrf52.hex
	@echo Flashing: $(BIN_DIR)/xs_nrf52.hex
	nrfjprog -f nrf52 --program $(BIN_DIR)/xs_nrf52.hex --sectorerase
	nrfjprog -f nrf52 --reset

erase:
	nrfjprog -f nrf52 --eraseall

$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# library xs:" $(<F) "(strings in flash)"
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# library: " $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@

$(LIB_DIR)/%.S.o %.s.o.o: %.S
	@echo "# asm " $(<F)
	$(CC) -c -x assembler-with-cpp $(ASMFLAGS) $(C_INCLUDES) $< -o $@

$(TMP_DIR)/%.c.o: %.c
	@echo "# application: " $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.asm
	$(AS) $(NRF_ASM_FLAGS) $@.asm -o $@
#	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@.unmapped
#	$(OBJCOPY) --rename-section .data.gxKeys=.rodata.gxKeys --rename-section .data.gxNames=.rodata.gxNames --rename-section .data.gxGlobals=.rodata.gxGlobals $@.unmapped $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(RESOURCES) -o $(TMP_DIR) -p nrf52 -r mc.resources.c

MAKEFLAGS += --jobs 1
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

