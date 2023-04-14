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

XS_GIT_VERSION ?= $(shell git -C $(MODDABLE) describe --tags --always --dirty 2> /dev/null)

PICO_ROOT ?= $(HOME)/pico
PICO_SDK_DIR ?= $(HOME)/pico/pico-sdk
PICO_GCC_ROOT ?= /usr/local

PIOASM ?= $(HOME)/pico/pico-sdk/build/pioasm/pioasm

PLATFORM_DIR = $(MODDABLE)/build/devices/pico

TOOLS_BIN = $(PICO_GCC_ROOT)/bin
TOOLS_PREFIX = arm-none-eabi-

DEBUGGER_SPEED ?= 115200
DEBUGGER_PORT ?= /dev/cu.SLAB_USBtoUART

XSBUG_HOST ?= localhost
XSBUG_PORT ?= 5002

UF2_VOLUME_NAME ?= RPI-RP2
PICO_VID ?= 2e8a
PICO_PID ?= 000a

UF2CONV = $(PICO_SDK_DIR)/build/elf2uf2/elf2uf2

# spot-check installation
ifeq ($(wildcard $(PICO_ROOT)),)
$(error Pico tools directory not found at $$PICO_ROOT: $(PICO_ROOT). Set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif
ifeq ($(wildcard $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc),)
$(error Pico GCC tools for "$(TOOLS_PREFIX)" not found at $$PICO_GCC_ROOT/bin: $(PICO_GCC_ROOT). Set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif
ifeq ($(wildcard $(PICO_SDK_DIR)),)
$(error Pico SDK directory not found at $$PICO_SDK_DIR: $(PICO_SDK_DIR). Set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif
ifeq ($(wildcard $(PICO_ROOT)/pico-examples),)
$(error Pico examples directory not found at $(PICO_ROOT)/pico-examples. Set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif
ifeq ($(wildcard $(PIOASM)),)
$(error Pico pioasm not found at $$PIOASM: $(PIOASM). Update instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif
ifeq ($(shell which cmake),)
$(error cmake not found. Set-up instructions at https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/pico.md)
endif

ifeq ($(HOST_OS),Darwin)
	DO_COPY = cp $(BIN_DIR)/xs_pico.uf2 $(UF2_VOLUME_PATH)
	MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/release
	UF2_VOLUME_PATH = /Volumes/$(UF2_VOLUME_NAME)

	PROGRAMMING_MODE = $(PLATFORM_DIR)/config/programmingMode $(PICO_VID) $(PICO_PID) $(UF2_VOLUME_PATH)

	ifeq ($(DEBUG),1)
		ifeq ($(XSBUG_LOG),1)
			DO_XSBUG =
			CONNECT_XSBUG=@echo "Connect to xsbug-log @ $(PICO_VID):$(PICO_PID)." && export XSBUG_PORT=$(XSBUG_PORT) && export XSBUG_HOST=$(XSBUG_HOST) && cd $(MODDABLE)/tools/xsbug-log && node xsbug-log serial2xsbug $(PICO_VID):$(PICO_PID) $(DEBUGGER_SPEED) 8N1
		else
			DO_XSBUG = open -a $(MODDABLE_TOOLS_DIR)/xsbug.app -g
			CONNECT_XSBUG=@echo "Connect to xsbug @ $(PICO_VID):$(PICO_PID)." ; export XSBUG_PORT=$(XSBUG_PORT) ; export XSBUG_HOST=$(XSBUG_HOST) ; serial2xsbug $(PICO_VID):$(PICO_PID) $(DEBUGGER_SPEED) 8N1
		endif
#		NORESTART=-norestart
#		WAIT_FOR_COPY_COMPLETE = $(PLATFORM_DIR)/config/waitForVolume -x $(UF2_VOLUME_PATH)
	else
		DO_XSBUG =
		CONNECT_XSBUG =
		NORESTART =
		WAIT_FOR_COPY_COMPLETE = $(PLATFORM_DIR)/config/waitForVolume -x $(UF2_VOLUME_PATH)
	endif

### Linux
else
	DO_COPY = DESTINATION=$$(cat $(TMP_DIR)/volumename); cp $(BIN_DIR)/xs_pico.uf2 $$DESTINATION
	MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/lin/release

#	PROGRAMMING_MODE = $(PLATFORM_DIR)/config/waitForVolumeLinux $(UF2_VOLUME_PATH)
	PROGRAMMING_MODE = PATH=$(PLATFORM_DIR)/config:$(PATH) ; programmingModeLinux $(PICO_VID) $(PICO_PID) $(UF2_VOLUME_NAME) $(TMP_DIR)/volumename
	WAIT_FOR_COPY_COMPLETE = $(PLATFORM_DIR)/config/waitForVolumeLinux -x $(UF2_VOLUME_NAME) $(TMP_DIR)/volumename

	ifeq ($(DEBUG),1)
		ifeq ($(XSBUG_LOG),1)
			DO_XSBUG =
		else
			DO_XSBUG = $(shell nohup $(MODDABLE_TOOLS_DIR)/xsbug > /dev/null 2>&1 &)
		endif
		CONNECT_XSBUG = PATH=$(PLATFORM_DIR)/config:$(PATH) ; $(PLATFORM_DIR)/config/connectToXsbugLinux $(PICO_VID) $(PICO_PID) $(XSBUG_LOG)
#		NORESTART=-norestart
	else
		DO_XSBUG =
		CONNECT_XSBUG =
		NORESTART =
	endif
endif
KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)

HW_DEBUG_OPT = $(FP_OPTS) # -flto
HW_OPT = -O2 $(FP_OPTS) # -flto

ifeq ($(MAKEFLAGS_JOBS),)
	MAKEFLAGS_JOBS = --jobs 8
endif

# Assembler flags common to all targets
ASMFLAGS += -mcpu=cortex-m0
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += $(FP_OPTS)

# Linker flags

LDFLAGS += \
	-mthumb						\
	-march=armv6-m				\
	-mcpu=cortex-m0plus			\
	-Wl,--build-id=none			\
	--specs=nosys.specs			\

LDFLAGS += \
	-Wl,--wrap=sprintf			\
	-Wl,--wrap=snprintf			\
	-Wl,--wrap=vsnprintf		\
	-Wl,--wrap=__clzsi2			\
	-Wl,--wrap=__clzdi2			\
	-Wl,--wrap=__ctzsi2			\
	-Wl,--wrap=__ctzdi2			\
	-Wl,--wrap=__popcountsi2			\
	-Wl,--wrap=__popcountdi2			\
	-Wl,--wrap=__clz			\
	-Wl,--wrap=__clzl			\
	-Wl,--wrap=__clzll			\
	-Wl,--wrap=__aeabi_idiv			\
	-Wl,--wrap=__aeabi_idivmod			\
	-Wl,--wrap=__aeabi_ldivmod			\
	-Wl,--wrap=__aeabi_uidiv			\
	-Wl,--wrap=__aeabi_uidivmod			\
	-Wl,--wrap=__aeabi_uldivmod			\
	-Wl,--wrap=__aeabi_dadd			\
	-Wl,--wrap=__aeabi_ddiv			\
	-Wl,--wrap=__aeabi_dmul			\
	-Wl,--wrap=__aeabi_drsub			\
	-Wl,--wrap=__aeabi_dsub			\
	-Wl,--wrap=__aeabi_cdcmpeq			\
	-Wl,--wrap=__aeabi_cdrcmple			\
	-Wl,--wrap=__aeabi_cdcmple			\
	-Wl,--wrap=__aeabi_dcmpeq			\
	-Wl,--wrap=__aeabi_dcmplt			\
	-Wl,--wrap=__aeabi_dcmple			\
	-Wl,--wrap=__aeabi_dcmpge			\
	-Wl,--wrap=__aeabi_dcmpgt			\
	-Wl,--wrap=__aeabi_dcmpun			\
	-Wl,--wrap=__aeabi_i2d			\
	-Wl,--wrap=__aeabi_l2d			\
	-Wl,--wrap=__aeabi_ui2d			\
	-Wl,--wrap=__aeabi_ul2d			\
	-Wl,--wrap=__aeabi_d2iz			\
	-Wl,--wrap=__aeabi_d2lz			\
	-Wl,--wrap=__aeabi_d2uiz			\
	-Wl,--wrap=__aeabi_d2ulz			\
	-Wl,--wrap=__aeabi_d2f			\
	-Wl,--wrap=sqrt			\
	-Wl,--wrap=cos			\
	-Wl,--wrap=sin			\
	-Wl,--wrap=tan			\
	-Wl,--wrap=atan2			\
	-Wl,--wrap=exp			\
	-Wl,--wrap=log			\
	-Wl,--wrap=ldexp			\
	-Wl,--wrap=copysign			\
	-Wl,--wrap=trunc			\
	-Wl,--wrap=floor			\
	-Wl,--wrap=ceil			\
	-Wl,--wrap=round			\
	-Wl,--wrap=sincos			\
	-Wl,--wrap=asin			\
	-Wl,--wrap=acos			\
	-Wl,--wrap=atan			\
	-Wl,--wrap=sinh			\
	-Wl,--wrap=cosh			\
	-Wl,--wrap=tanh			\
	-Wl,--wrap=asinh			\
	-Wl,--wrap=acosh			\
	-Wl,--wrap=atanh			\
	-Wl,--wrap=exp2			\
	-Wl,--wrap=log2			\
	-Wl,--wrap=exp10			\
	-Wl,--wrap=log10			\
	-Wl,--wrap=pow			\
	-Wl,--wrap=powint			\
	-Wl,--wrap=hypot			\
	-Wl,--wrap=cbrt			\
	-Wl,--wrap=fmod			\
	-Wl,--wrap=drem			\
	-Wl,--wrap=remainder			\
	-Wl,--wrap=remquo			\
	-Wl,--wrap=expm1			\
	-Wl,--wrap=log1p			\
	-Wl,--wrap=fma			\
	-Wl,--wrap=__aeabi_lmul			\
	-Wl,--wrap=__aeabi_fadd			\
	-Wl,--wrap=__aeabi_fdiv			\
	-Wl,--wrap=__aeabi_fmul			\
	-Wl,--wrap=__aeabi_frsub			\
	-Wl,--wrap=__aeabi_fsub			\
	-Wl,--wrap=__aeabi_cfcmpeq			\
	-Wl,--wrap=__aeabi_cfrcmple			\
	-Wl,--wrap=__aeabi_cfcmple			\
	-Wl,--wrap=__aeabi_fcmpeq			\
	-Wl,--wrap=__aeabi_fcmplt			\
	-Wl,--wrap=__aeabi_fcmple			\
	-Wl,--wrap=__aeabi_fcmpge			\
	-Wl,--wrap=__aeabi_fcmpgt			\
	-Wl,--wrap=__aeabi_fcmpun			\
	-Wl,--wrap=__aeabi_i2f			\
	-Wl,--wrap=__aeabi_l2f			\
	-Wl,--wrap=__aeabi_ui2f			\
	-Wl,--wrap=__aeabi_ul2f			\
	-Wl,--wrap=__aeabi_f2iz			\
	-Wl,--wrap=__aeabi_f2lz			\
	-Wl,--wrap=__aeabi_f2uiz			\
	-Wl,--wrap=__aeabi_f2ulz			\
	-Wl,--wrap=__aeabi_f2d			\
	-Wl,--wrap=sqrtf			\
	-Wl,--wrap=cosf			\
	-Wl,--wrap=sinf			\
	-Wl,--wrap=tanf			\
	-Wl,--wrap=atan2f			\
	-Wl,--wrap=expf			\
	-Wl,--wrap=logf			\
	-Wl,--wrap=ldexpf			\
	-Wl,--wrap=copysignf			\
	-Wl,--wrap=truncf			\
	-Wl,--wrap=floorf			\
	-Wl,--wrap=ceilf			\
	-Wl,--wrap=roundf			\
	-Wl,--wrap=sincosf			\
	-Wl,--wrap=asinf			\
	-Wl,--wrap=acosf			\
	-Wl,--wrap=atanf			\
	-Wl,--wrap=sinhf			\
	-Wl,--wrap=coshf			\
	-Wl,--wrap=tanhf			\
	-Wl,--wrap=asinhf			\
	-Wl,--wrap=acoshf			\
	-Wl,--wrap=atanhf			\
	-Wl,--wrap=exp2f			\
	-Wl,--wrap=log2f			\
	-Wl,--wrap=exp10f			\
	-Wl,--wrap=log10f			\
	-Wl,--wrap=powf			\
	-Wl,--wrap=powintf			\
	-Wl,--wrap=hypotf			\
	-Wl,--wrap=cbrtf			\
	-Wl,--wrap=fmodf			\
	-Wl,--wrap=dremf			\
	-Wl,--wrap=remainderf			\
	-Wl,--wrap=remquof			\
	-Wl,--wrap=expm1f			\
	-Wl,--wrap=log1pf			\
	-Wl,--wrap=fmaf			\
	-Wl,--wrap=malloc			\
	-Wl,--wrap=calloc			\
	-Wl,--wrap=realloc			\
	-Wl,--wrap=free			\
	-Wl,--wrap=memcpy			\
	-Wl,--wrap=memset			\
	-Wl,--wrap=__aeabi_memcpy			\
	-Wl,--wrap=__aeabi_memset			\
	-Wl,--wrap=__aeabi_memcpy4			\
	-Wl,--wrap=__aeabi_memset4			\
	-Wl,--wrap=__aeabi_memcpy8			\
	-Wl,--wrap=__aeabi_memset8			\
	-Wl,-Map=$(BIN_DIR)/xs_pico.map		\
	-Wl,--script=$(LINKER_SCRIPT)		\
	-Wl,-z,max-page-size=4096	\
	-Wl,--gc-sections		\
	-Wl,--wrap=printf			\
	-Wl,--wrap=vprintf			\
	-Wl,--wrap=puts				\
	-Wl,--wrap=putchar			\
	-Wl,--wrap=getchar			\


LIB_FILES += \
	-lc -lnosys -lm 

INC_DIRS = \
	$(TMP_DIR)	\
	$(PICO_SDK_DIR)/src/boards/include	\
	$(PICO_SDK_DIR)/src/common/pico_base/include	\
	$(PICO_SDK_DIR)/src/common/pico_binary_info/include	\
	$(PICO_SDK_DIR)/src/common/pico_bit_ops/include	\
	$(PICO_SDK_DIR)/src/common/pico_divider/include	\
	$(PICO_SDK_DIR)/src/common/pico_stdlib/include	\
	$(PICO_SDK_DIR)/src/common/pico_sync/include	\
	$(PICO_SDK_DIR)/src/common/pico_time/include	\
	$(PICO_SDK_DIR)/src/common/pico_usb_reset_interface/include		\
	$(PICO_SDK_DIR)/src/common/pico_util/include	\
	$(PICO_SDK_DIR)/src/rp2_common/boot_stage2/include		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_adc/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_gpio/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_i2c/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_spi/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pwm/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_base/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_claim/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_sync/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_uart/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_divider/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_timer/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_clocks/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pio/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_dma/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_resets/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_watchdog/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_xosc/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pll/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_vreg/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_irq/include	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_flash/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_bootrom/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_double/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_fix/rp2040_usb_device_enumeration/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_float/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_int64_ops/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_malloc/include		\
	$(PICO_SDK_DIR)/src/rp2_common/pico_printf/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_platform/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_runtime/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio_usb/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_unique_id/include	\
	$(PICO_SDK_DIR)/src/rp2040/hardware_regs/include	\
	$(PICO_SDK_DIR)/src/rp2040/hardware_structs/include	\
	$(PICO_SDK_DIR)/lib/tinyusb/src		\
	$(PICO_SDK_DIR)/lib/tinyusb/src/common		\
	$(PICO_SDK_DIR)/lib/tinyusb/hw		\
	$(XS_DIR)/../modules/files/preference \
	$(XS_DIR)/../modules/base/instrumentation \
	$(XS_DIR)/../modules/base/timer \
	$(PLATFORM_DIR)	\
	$(PLATFORM_DIR)/base \
	$(PLATFORM_DIR)/config \
	$(LIB_DIR)

#	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio_uart/include		\

INC_DIRS += \
	$(PICO_SDK_DIR)/src/rp2_common/pico_cyw43_arch/include	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_cyw43_driver/include \
	$(PICO_SDK_DIR)/src/rp2_common/pico_lwip/include \
	$(PICO_SDK_DIR)/src/rp2_common/pico_rand/include \
	$(PICO_SDK_DIR)/src/rp2_common/pico_async_context/include \
	$(PICO_SDK_DIR)/lib/cyw43-driver/src	\
	$(PICO_SDK_DIR)/lib/cyw43-driver/firmware	\
	$(PICO_SDK_DIR)/lib/lwip/src/include

XS_OBJ = \
	$(LIB_DIR)/xsHosts.c.o \
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
	$(XS_DIR)/platforms/pico \
	$(XS_DIR)/platforms/mc \
	$(BUILD_DIR)/devices/pico

XS_HEADERS = \
	$(XS_DIR)/includes/xs.h \
	$(XS_DIR)/includes/xsmc.h \
	$(XS_DIR)/sources/xsAll.h \
	$(XS_DIR)/sources/xsCommon.h \
	$(XS_DIR)/platforms/pico/xsPlatform.h \
	$(XS_DIR)/platforms/pico/xsHost.h \
	$(XS_DIR)/platforms/mc/xsHosts.h

HEADERS += $(XS_HEADERS)

PICO_OBJ = \
	$(LIB_DIR)/adc.c.o \
	$(LIB_DIR)/async_context_base.c.o \
	$(LIB_DIR)/async_context_poll.c.o \
	$(LIB_DIR)/binary_info.c.o \
	$(LIB_DIR)/bit_ops_aeabi.S.o \
	$(LIB_DIR)/bootrom.c.o \
	$(LIB_DIR)/bs2_default_padded_checksummed.S.o \
	$(LIB_DIR)/claim.c.o \
	$(LIB_DIR)/clocks.c.o \
	$(LIB_DIR)/crt0.S.o \
	$(LIB_DIR)/critical_section.c.o \
	$(LIB_DIR)/datetime.c.o \
	$(LIB_DIR)/dma.c.o	\
	$(LIB_DIR)/double_aeabi.S.o \
	$(LIB_DIR)/double_init_rom.c.o \
	$(LIB_DIR)/double_math.c.o \
	$(LIB_DIR)/double_v1_rom_shim.S.o \
	$(LIB_DIR)/flash.c.o \
	$(LIB_DIR)/float_aeabi.S.o \
	$(LIB_DIR)/float_init_rom.c.o \
	$(LIB_DIR)/float_math.c.o \
	$(LIB_DIR)/float_v1_rom_shim.S.o \
	$(LIB_DIR)/gpio.c.o \
 	$(LIB_DIR)/hardware_divider.S.o \
	$(LIB_DIR)/i2c.c.o \
	$(LIB_DIR)/irq.c.o \
	$(LIB_DIR)/irq_handler_chain.S.o \
	$(LIB_DIR)/lock_core.c.o \
	$(LIB_DIR)/mem_ops_aeabi.S.o \
	$(LIB_DIR)/mutex.c.o \
	$(LIB_DIR)/pheap.c.o \
	$(LIB_DIR)/pico_divider.S.o \
	$(LIB_DIR)/pico_int64_ops_aeabi.S.o \
	$(LIB_DIR)/pico_malloc.c.o \
	$(LIB_DIR)/pio.c.o	\
	$(LIB_DIR)/platform.c.o \
	$(LIB_DIR)/pll.c.o \
	$(LIB_DIR)/printf.c.o \
	$(LIB_DIR)/queue.c.o \
	$(LIB_DIR)/rand.c.o \
	$(LIB_DIR)/reset_interface.c.o \
	$(LIB_DIR)/rp2040_usb_device_enumeration.c.o \
	$(LIB_DIR)/runtime.c.o \
	$(LIB_DIR)/sem.c.o \
	$(LIB_DIR)/spi.c.o \
	$(LIB_DIR)/stdlib.c.o \
	$(LIB_DIR)/sync.c.o \
	$(LIB_DIR)/time.c.o \
	$(LIB_DIR)/timeout_helper.c.o \
	$(LIB_DIR)/timer.c.o \
	$(LIB_DIR)/vreg.c.o \
	$(LIB_DIR)/watchdog.c.o \
	$(LIB_DIR)/xosc.c.o \
	\
	$(LIB_DIR)/stdio.c.o \
	$(LIB_DIR)/stdio_usb.c.o \
	$(LIB_DIR)/stdio_usb_descriptors.c.o \
	$(LIB_DIR)/dcd_rp2040.c.o \
	$(LIB_DIR)/rp2040_usb.c.o \
	$(LIB_DIR)/usbd.c.o \
	$(LIB_DIR)/usbd_control.c.o \
	$(LIB_DIR)/cdc_device.c.o \
	$(LIB_DIR)/vendor_device.c.o \
	$(LIB_DIR)/tusb.c.o \
	$(LIB_DIR)/tusb_fifo.c.o \
 	$(LIB_DIR)/uart.c.o \
 	$(LIB_DIR)/unique_id.c.o

#	$(LIB_DIR)/divider.S.o \
#	$(LIB_DIR)/dfu_rt_device.c.o \
#	$(LIB_DIR)/msc_device.c.o \
#	$(LIB_DIR)/stdio_uart.c.o \

LWIP_OBJ = \
	$(LIB_DIR)/cyw43_lwip.c.o	\
	$(LIB_DIR)/def.c.o	\
	$(LIB_DIR)/dns.c.o	\
	$(LIB_DIR)/init.c.o	\
	$(LIB_DIR)/inet_chksum.c.o	\
	$(LIB_DIR)/ip.c.o	\
	$(LIB_DIR)/mem.c.o	\
	$(LIB_DIR)/memp.c.o	\
	$(LIB_DIR)/netif.c.o	\
	$(LIB_DIR)/pbuf.c.o	\
	$(LIB_DIR)/raw.c.o	\
	$(LIB_DIR)/stats.c.o	\
	$(LIB_DIR)/sys.c.o	\
	$(LIB_DIR)/altcp.c.o	\
	$(LIB_DIR)/altcp_alloc.c.o	\
	$(LIB_DIR)/altcp_tcp.c.o	\
	$(LIB_DIR)/tcp.c.o	\
	$(LIB_DIR)/tcp_in.c.o	\
	$(LIB_DIR)/tcp_out.c.o	\
	$(LIB_DIR)/udp.c.o	\
	$(LIB_DIR)/acd.c.o	\
	$(LIB_DIR)/autoip.c.o	\
	$(LIB_DIR)/dhcp.c.o	\
	$(LIB_DIR)/etharp.c.o	\
	$(LIB_DIR)/icmp.c.o	\
	$(LIB_DIR)/igmp.c.o	\
	$(LIB_DIR)/ip4_frag.c.o	\
	$(LIB_DIR)/ip4.c.o	\
	$(LIB_DIR)/ip4_addr.c.o	\
	$(LIB_DIR)/api_lib.c.o	\
	$(LIB_DIR)/api_msg.c.o	\
	$(LIB_DIR)/err.c.o	\
	$(LIB_DIR)/if_api.c.o	\
	$(LIB_DIR)/netbuf.c.o	\
	$(LIB_DIR)/netdb.c.o	\
	$(LIB_DIR)/netifapi.c.o	\
	$(LIB_DIR)/sockets.c.o	\
	$(LIB_DIR)/tcpip.c.o	\
	$(LIB_DIR)/ethernet.c.o	\
	$(LIB_DIR)/bridgeif.c.o	\
	$(LIB_DIR)/bridgeif_fdb.c.o	\
	$(LIB_DIR)/slipif.c.o	\
	$(LIB_DIR)/zepif.c.o	\
	$(LIB_DIR)/timeouts.c.o	\

#	$(LIB_DIR)/cyw43_resource.o

PICO_OBJ += \
	$(LWIP_OBJ)	\
	$(LIB_DIR)/cyw43_arch.c.o	\
	$(LIB_DIR)/cyw43_bus_pio_spi.c.o	\
	$(LIB_DIR)/cyw43_ctrl.c.o	\
	$(LIB_DIR)/cyw43_driver.c.o	\
	$(LIB_DIR)/cyw43_ll.c.o	\
	$(LIB_DIR)/cyw43_stats.c.o	\
	$(LIB_DIR)/lwip_nosys.c.o	\
	$(LIB_DIR)/cyw43_arch_poll.c.o


#	$(LIB_DIR)/cyw43_arch_threadsafe_background.c.o

PICO_SRC_DIRS = \
	$(PICO_SDK_DIR)/src/common/pico_sync				\
	$(PICO_SDK_DIR)/src/common/pico_time				\
	$(PICO_SDK_DIR)/src/common/pico_util				\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_dma			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pio			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_adc			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_gpio		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pwm			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_i2c			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_spi			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_claim		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_sync		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_flash		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_uart		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_divider		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_timer		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_clocks		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_watchdog	\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_xosc		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_pll			\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_vreg		\
	$(PICO_SDK_DIR)/src/rp2_common/hardware_irq			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_async_context	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_bit_ops			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_bootrom			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_double			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_int64_ops		\
	$(PICO_SDK_DIR)/src/rp2_common/pico_float			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_malloc			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_mem_ops			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_printf			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_platform		\
	$(PICO_SDK_DIR)/src/rp2_common/pico_rand				\
	$(PICO_SDK_DIR)/src/rp2_common/pico_runtime			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_stdlib			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_standard_link	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio_usb		\
	$(PICO_SDK_DIR)/src/rp2_common/boot_stage2			\
	$(PICO_SDK_DIR)/lib/tinyusb/src/portable/raspberrypi/rp2040		\
	$(PICO_SDK_DIR)/lib/tinyusb/src/device				\
	$(PICO_SDK_DIR)/lib/tinyusb/src/class/cdc			\
	$(PICO_SDK_DIR)/lib/tinyusb/src/class/vendor		\
	$(PICO_SDK_DIR)/lib/tinyusb/src						\
	$(PICO_SDK_DIR)/lib/tinyusb/src/common				\
	$(PICO_SDK_DIR)/lib/tinyusb/hw						\
	$(PICO_SDK_DIR)/src/rp2_common/pico_fix/rp2040_usb_device_enumeration	\
	$(PICO_SDK_DIR)/src/rp2_common/pico_unique_id

PICO_SRC_DIRS += \
	$(PICO_SDK_DIR)/lib/cyw43-driver/src				\
	$(PICO_SDK_DIR)/lib/cyw43-driver					\
	$(PICO_SDK_DIR)/lib/lwip/src/core					\
	$(PICO_SDK_DIR)/lib/lwip/src/core/ipv4				\
	$(PICO_SDK_DIR)/lib/lwip/src/api					\
	$(PICO_SDK_DIR)/lib/lwip/src/netif					\
	$(PICO_SDK_DIR)/src/rp2_common/pico_lwip			\
	$(PICO_SDK_DIR)/src/rp2_common/pico_cyw43_arch		\
	$(PICO_SDK_DIR)/src/rp2_common/pico_cyw43_driver	\

PIO_STUFF += \
	$(TMP_DIR)/cyw43_bus_pio_spi.pio.h


PIO_STUFF += \
	$(TMP_DIR)/ws2812.pio.h

PICO_SRC_DIRS += \
	$(BUILD_DIR)/devices/pico/pio  \

#	$(PICO_SDK_DIR)/lib/tinyusb/src/class/msc			\
#	$(PICO_SDK_DIR)/lib/tinyusb/src/class/dfu			\
#	$(PICO_SDK_DIR)/src/rp2_common/pico_stdio_uart		\

SDK_GLUE_OBJ = \
	$(TMP_DIR)/xsmain.c.o \
	$(TMP_DIR)/debugger.c.o \
	$(TMP_DIR)/main.c.o

SDK_GLUE_DIRS = \
	$(BUILD_DIR)/devices/pico/base  \
	$(BUILD_DIR)/devices/pico/config 

OBJECTS += \
	$(PICO_OBJ)

OTHER_STUFF += \
	env_vars	\
	pio_stuff


CC  = $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc
CPP = $(TOOLS_BIN)/$(TOOLS_PREFIX)g++
# LD  = $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc
LD  = $(TOOLS_BIN)/$(TOOLS_PREFIX)g++
AR  = $(TOOLS_BIN)/$(TOOLS_PREFIX)ar
OBJCOPY = $(TOOLS_BIN)/$(TOOLS_PREFIX)objcopy
OBJDUMP = $(TOOLS_BIN)/$(TOOLS_PREFIX)objdump
SIZE  = $(TOOLS_BIN)/$(TOOLS_PREFIX)size

AR_FLAGS = crs

BUILDCLUT = $(MODDABLE_TOOLS_DIR)/buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)/compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)/rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)/mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)/mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)/png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)/image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)/wav2maud
XSC = $(MODDABLE_TOOLS_DIR)/xsc
XSID = $(MODDABLE_TOOLS_DIR)/xsid
XSL = $(MODDABLE_TOOLS_DIR)/xsl

#	-DmxNoConsole=1
#	-DPICO_DEBUG_MALLOC=1

PICO_SDK_DEFINES= \
	-DPICO_STDIO_ENABLE_CRLF_SUPPORT=0 \
	-DPICO_STDIO_USB_DEFAULT_CRLF=0 \
	-DPICO_STDIO_DEFAULT_CRLF=0 \
	-DPICO_DEBUG_MALLOC_LOW_WATER=0	\
	-DPICO_DEFAULT_UART_BAUD_RATE=$(DEBUGGER_SPEED) \
	-DPICO_HEAP_SIZE=0xC000

PICO_C_DEFINES= \
	$(PICO_SDK_DEFINES) \
	-DCFG_TUSB_DEBUG=0 \
	-DCFG_TUSB_MCU=OPT_MCU_RP2040 \
	-DCFG_TUSB_OS=OPT_OS_PICO \
	-DLIB_PICO_BIT_OPS=1	\
	-DLIB_PICO_BIT_OPS_PICO=1	\
	-DLIB_PICO_DIVIDER=1	\
	-DLIB_PICO_DIVIDER_HARDWARE=1	\
	-DLIB_PICO_DOUBLE=1	\
	-DLIB_PICO_DOUBLE_PICO=1	\
	-DLIB_PICO_FLOAT=1	\
	-DLIB_PICO_FLOAT_PICO=1	\
	-DLIB_PICO_INT64_OPS=1	\
	-DLIB_PICO_INT64_OPS_PICO=1	\
	-DLIB_PICO_MALLOC=1	\
	-DLIB_PICO_MEM_OPS_PICO=1	\
	-DLIB_PICO_PLATFORM=1	\
	-DLIB_PICO_PRINTF=1	\
	-DLIB_PICO_PRINTF_PICO=1	\
	-DLIB_PICO_RAND=1	\
	-DLIB_PICO_RUNTIME=1	\
	-DLIB_PICO_STANDARD_LINK=1	\
	-DLIB_PICO_STDIO=1	\
	-DLIB_PICO_STDIO_USB=1	\
	-DLIB_PICO_STDLIB=1	\
	-DLIB_PICO_SYNC=1	\
	-DLIB_PICO_SYNC_CORE=1	\
	-DLIB_PICO_SYNC_CRITICAL_SECTION=1	\
	-DLIB_PICO_SYNC_MUTEX=1	\
	-DLIB_PICO_SYNC_SEM=1	\
	-DLIB_PICO_TIME=1	\
	-DLIB_PICO_UNIQUE_ID=1	\
	-DLIB_PICO_UTIL=1	\
	-DPICO_BUILD=1	\
	-DPICO_COPY_TO_RAM=0	\
	-DPICO_CXX_ENABLE_EXCEPTIONS=0	\
	-DPICO_NO_FLASH=0	\
	-DPICO_NO_HARDWARE=0	\
	-DPICO_ON_DEVICE=1	\
	-DPICO_PROGRAM_URL=\"https://github.com/Moddable-OpenSource\"	\
	-DPICO_TARGET_NAME=\"$(NAME)\"	\
	-DPICO_USE_BLOCKED_RAM=0 \
	-DLIB_PICO_FIX_RP2040_USB_DEVICE_ENUMERATION=1	\

PICO_C_DEFINES += \
	-DCYW43_LWIP=1				\
	-DLIB_PICO_CYW43_ARCH=1		\
	-DPICO_CYW43_ARCH_POLL=1

#	-DPICO_CYW43_ARCH_THREADSAFE_BACKGROUND=1

ifeq ($(WIFI_GPIO),1)
PICO_C_DEFINES += \
	-DPICO_BOARD=\"pico_w\"		\
	-DWIFI_GPIO=1
else
PICO_C_DEFINES += \
	-DPICO_BOARD=\"pico\"
endif
BOARD_INCLUDE = -include $(PICO_SDK_DIR)/src/boards/include/boards/pico_w.h

C_DEFINES = \
	$(PICO_C_DEFINES) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION) \
	-DMODGCC=1

#	--sysroot=$(NRF52_GCC_ROOT)/arm-none-eabi 

C_FLAGS=\
	-c	\
	-std=gnu11 \
	-march=armv6-m	\
	-mcpu=cortex-m0plus	\
	-mthumb	\
	-ffunction-sections -fdata-sections	\

ifeq ($(DEBUG),1)
	C_DEFINES += \
		-DDEBUG=1 \
		-DmxDebug=1 \
		-g3 \
		-Os
	C_FLAGS += $(HW_DEBUG_OPT) $(BOARD_INCLUDE)
	ASM_FLAGS += $(HW_DEBUG_OPT) $(BOARD_INCLUDE)
else
	C_DEFINES += \
		-DNDEBUG	\
		-Os
	C_FLAGS += $(HW_OPT) $(BOARD_INCLUDE)
	ASM_FLAGS += $(HW_OPT) $(BOARD_INCLUDE)
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

C_FLAGS_NODATASECTION = $(C_FLAGS)

LINKER_SCRIPT := $(PLATFORM_DIR)/config/xsproj.ld

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
SRC_GIT_VERSION = $(call git_description,$(PICO_SDK_DIR)/sources)
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

VPATH += $(PICO_SRC_DIRS) $(PIO_DIRS) $(SDK_GLUE_DIRS) $(XS_DIRS)

.PHONY: all	
.SUFFIXES:
%.d:
.PRECIOUS: %.d %.o

all: precursor $(BIN_DIR)/xs_pico.uf2
	$(KILL_SERIAL_2_XSBUG)
	$(PROGRAMMING_MODE)
	$(DO_XSBUG)
	@echo Copying: $(BIN_DIR)/xs_pico.elf to $(UF2_VOLUME_NAME)
	$(DO_COPY)
	$(WAIT_FOR_COPY_COMPLETE)
#	$(CONNECT_XSBUG)
	$(CONNECT_XSBUG) $(NORESTART)

deploy: precursor $(BIN_DIR)/xs_pico.uf2
	$(KILL_SERIAL_2_XSBUG)
	$(PROGRAMMING_MODE)
	@echo Copying: $(BIN_DIR)/xs_pico.elf to $(UF2_VOLUME_NAME)
	$(DO_COPY)
	$(WAIT_FOR_COPY_COMPLETE)

build: precursor $(BIN_DIR)/xs_pico.uf2
	@echo Target built: $(BIN_DIR)/xs_pico.uf2

precursor: $(TMP_DIR) $(LIB_DIR) $(OTHER_STUFF) $(BIN_DIR)/xs_pico.elf

env_vars:
ifndef PICO_SDK_DIR
	$(error PICO_SDK_DIR environment variable must be defined! See https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/ for details.)
endif

pio_stuff: $(PIO_STUFF)

$(MODDABLE_TOOLS_DIR)/findUSBLinux: $(PLATFORM_DIR)/config/findUSBLinux
	cp $(PLATFORM_DIR)/config/findUSBLinux $(MODDABLE_TOOLS_DIR)

clean:
	echo "# Clean project"
	-rm -rf $(BIN_DIR) 2>/dev/null
	-rm -rf $(TMP_DIR) 2>/dev/null
	-rm -rf $(LIB_DIR) 2>/dev/null

allclean:
	@echo "# Cleaning all pico"
	@echo "# rm $(MODDABLE)/build/bin/pico"
	-rm -rf $(MODDABLE)/build/bin/pico
	@echo "# rm $(MODDABLE)/build/tmp/pico"
	-rm -rf $(MODDABLE)/build/tmp/pico

$(BIN_DIR)/xs_pico.uf2: $(BIN_DIR)/xs_pico.elf
	@echo Making: $(BIN_DIR)/xs_pico.uf2 from xs_pico.elf
	$(UF2CONV) $(BIN_DIR)/xs_pico.elf $(BIN_DIR)/xs_pico.uf2

xsbug:
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	$(CONNECT_XSBUG)

$(TMP_DIR):
	@echo "TMP_DIR"
	mkdir -p $(TMP_DIR)

$(LIB_DIR)/buildinfo.h:
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	
FINAL_LINK_OBJ:=\
	$(XS_OBJ) \
	$(SDK_GLUE_OBJ) \
	$(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o \
	$(OBJECTS) \
	$(LIB_DIR)/buildinfo.c.o

ekoFiles = $(foreach fil,$(FINAL_LINK_OBJ),$(shell echo '$(strip $(fil))' >> $(BIN_DIR)/xs_pico.ind1))

$(BIN_DIR)/xs_pico.ind: $(FINAL_LINK_OBJ)
	@echo "# creating xs_pico.ind"
#	 @echo "# FINAL LINK OBJ: $(FINAL_LINK_OBJ)"
	@rm -f $(BIN_DIR)/xs_pico.ind
#	@echo $(ekoFiles)
	$(ekoFiles)
	@mv $(BIN_DIR)/xs_pico.ind1 $(BIN_DIR)/xs_pico.ind

$(BIN_DIR)/xs_pico.elf: $(FINAL_LINK_OBJ)
	@echo "# creating xs_pico.elf"
#	 @echo "# FINAL LINK OBJ: $(FINAL_LINK_OBJ)"
	@rm -f $(BIN_DIR)/xs_pico.elf
	@echo "# link to .elf file"
	$(LD) $(LDFLAGS) $(FINAL_LINK_OBJ) $(LIB_FILES) -o $@
	@echo "# make .dis file"
	$(OBJDUMP) -h $(BIN_DIR)/xs_pico.elf > $(BIN_DIR)/xs_pico.dis
	$(OBJDUMP) -d $(BIN_DIR)/xs_pico.elf >> $(BIN_DIR)/xs_pico.dis
	$(OBJDUMP) -t $(BIN_DIR)/xs_pico.elf >> $(BIN_DIR)/xs_pico.sym


$(LIB_DIR)/buildinfo.c.o: $(SDK_GLUE_OBJ) $(XS_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS) $(LIB_DIR)/buildinfo.h
	@echo "# buildinfo"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.c
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $(LIB_DIR)/buildinfo.c -o $@

$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# library xs:" $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@

$(LIB_DIR)/%.c.o: %.c
	@echo "# library: " $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@

$(LIB_DIR)/%.S.o %.s.o: %.S
	@echo "# asm " $(<F)
	$(CC) -c -x assembler-with-cpp $(ASMFLAGS) $(C_INCLUDES) $< -o $@

$(TMP_DIR)/%.c.o: %.c
	@echo "# application: " $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@

$(TMP_DIR)/mc.%.c.o: $(TMP_DIR)/mc.%.c
	@echo "# cc" $(<F) "(slots in flash)"
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@

$(TMP_DIR)/mc.xs.c: $(MODULES) $(MANIFEST)
	@echo "# xsl modules"
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) $(MODULES)

$(TMP_DIR)/mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo "# mcrez resources"
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -p pico -r mc.resources.c

$(LIB_DIR)/hardware_divider.S.o: $(PICO_SDK_DIR)/src/rp2_common/hardware_divider/divider.S
	@echo "# asm (special) " $(PICO_SDK_DIR)/src/rp2_common/hardware_divider/divider.S
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $(LIB_DIR)/hardware_divider.S.o

$(LIB_DIR)/pico_divider.S.o: $(PICO_SDK_DIR)/src/rp2_common/pico_divider/divider.S
	@echo "# asm  (special)" $(<F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $(LIB_DIR)/pico_divider.S.o
#	$(CC) -c -x assembler-with-cpp $(ASMFLAGS) $(C_INCLUDES) $< -o $@

$(TMP_DIR)/cyw43_bus_pio_spi.pio.h: $(PICO_SDK_DIR)/src/rp2_common/pico_cyw43_driver/cyw43_bus_pio_spi.pio
	$(PIOASM) -o c-sdk $< $@

$(TMP_DIR)/%.pio.h: %.pio
	@echo "# compile pio: " $(<F)
	$(PIOASM) -o c-sdk $< $@


CYW43_FW_FILE=43439A0-7.95.49.00.combined
CYW43_FW_PATH=$(PICO_SDK_DIR)/lib/cyw43-driver/firmware
CYW43_FW_SYM=43439A0_7_95_49_00

$(LIB_DIR)/cyw43_resource.o: $(CYW43_FW_PATH)
	cd $(CYW43_FW_PATH) &&		\
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--readonly-text --rename-section .data=.big_const,contents,alloc,load,readonly,data \
		--redefine-sym _binary_$(CYW43_FW_SYM)_combined_start=fw_$(CYW43_FW_SYM)_start \
		--redefine-sym _binary_$(CYW43_FW_SYM)_combined_end=fw_$(CYW43_FW_SYM)_end \
		--redefine-sym _binary_$(CYW43_FW_SYM)_combined_size=fw_$(CYW43_FW_SYM)_size \
		$(CYW43_FW_FILE)	\
		$@

##@@ force to 1 while porting
# MAKEFLAGS_JOBS = --jobs 1
MAKEFLAGS += $(MAKEFLAGS_JOBS)
ifneq ($(VERBOSE),1)
MAKEFLAGS += --silent
endif

