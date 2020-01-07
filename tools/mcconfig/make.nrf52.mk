#
# Copyright (c) 2016-2019  Moddable Tech, Inc.
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

NRF_ROOT ?= $(HOME)/nRF5

PLATFORM_DIR = $(MODDABLE)/build/devices/nrf52

NRF_SERIAL_PORT ?= /dev/cu.usbmodem0000000000001
DEBUGGER_PORT ?= /dev/cu.usbserial-AL035YB2
DEBUGGER_SPEED ?= 115200

BOOTLOADER_HEX ?= $(PLATFORM_DIR)/bootloader/moddable_four_bootloader-0.2.13-21-g454b281_s140_6.1.1.hex
SOFTDEVICE_HEX ?= $(NRF_SDK_DIR)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex

UF2_VOLUME_NAME ?= MODDABLE4

GNU_VERSION ?= 8.2.1
# NRF52_GCC_ROOT ?= $(HOME)/opt/gcctoolchain
NRF52_GCC_ROOT ?= $(NRF_ROOT)/gcc-arm-none-eabi-8-2018-q4-major

NRF_SDK_DIR ?= $(NRF_ROOT)/nRF5_SDK
NRFJPROG ?= $(NRF_ROOT)/nrfjprog/nrfjprog
UF2CONV ?= $(NRF_ROOT)/uf2conv.py

# nRF52840_xxAA
BOARD = pca10056
SOFT_DEVICE = s140
SDK_ROOT = $(NRF_SDK_DIR)
HWCPU = cortex-m4

# BOARD_DEF = BOARD_PCA10056
# BOARD_DEF = BOARD_SPARKFUN_NRF52840_MINI
BOARD_DEF = BOARD_MODDABLE_FOUR

HEAP_SIZE = 0x13000

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
CFLAGS += -D$(BOARD_DEF)
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
ASMFLAGS += -D$(BOARD_DEF)
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
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# LDFLAGS += $(FP_OPTS)
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

LDFLAGS += -Xlinker -no-enum-size-warning -Xlinker -Map=$(BIN_DIR)/xs_lib.map
LIB_FILES += \
	-lc -lnosys -lm \
	$(SDK_ROOT)/external/nrf_cc310/lib/cortex-m4/hard-float/no-interrupts/libnrf_cc310_0.9.12.a

INC_DIRS = \
	$(NRF52_GCC_ROOT)/arm-none-eabi/include \
	$(NRF52_GCC_ROOT)/arm-none-eabi/include/machine \
	$(NRF52_GCC_ROOT)/lib/gcc/arm-none-eabi/$(GNU_VERSION)/include \
	$(NRF52_GCC_ROOT)/lib/gcc/arm-none-eabi/$(GNU_VERSION)/include-fixed \
	$(XS_DIR)/../modules/base/instrumentation \
	$(XS_DIR)/../modules/base/timer \
	$(BUILD_DIR)/devices/nrf52 \
	$(BUILD_DIR)/devices/nrf52/base \
	$(BUILD_DIR)/devices/nrf52/xsProj \

SDK_SRC=

FREE_RTOS_PATHS = \
	$(SDK_ROOT)/external/freertos/source \
	$(SDK_ROOT)/external/freertos/source/include \
	$(SDK_ROOT)/external/freertos/source/portable/MemMang \
	$(SDK_ROOT)/external/freertos/portable/GCC/nrf52 \
	$(SDK_ROOT)/external/freertos/portable/CMSIS/nrf52

CRYPTO_PATHS = \
  $(SDK_ROOT)/components/libraries/crypto \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310 \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_bl \
  $(SDK_ROOT)/components/libraries/crypto/backend/cifra \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_hw \
  $(SDK_ROOT)/components/libraries/crypto/backend/mbedtls \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_sw \
  $(SDK_ROOT)/components/libraries/crypto/backend/oberon \
  $(SDK_ROOT)/components/libraries/crypto/backend/optiga \
  $(SDK_ROOT)/external/nrf_cc310/include \
  $(SDK_ROOT)/external/nrf_cc310_bl/include \

# Include folders common to all targets
INC_DIRS += \
  $(CRYPTO_PATHS) \
  $(PLATFORM_DIR) \
  $(PLATFORM_DIR)/config \
  $(FREE_RTOS_PATHS) \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  $(SDK_ROOT)/components/libraries/atomic_flags \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/libraries/hardfault/nrf52 \
  $(SDK_ROOT)/components/libraries/hardfault/nrf52/handler \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/serial \
  $(SDK_ROOT)/components/libraries/spi_mngr \
  $(SDK_ROOT)/components/libraries/stack_info \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/libraries/twi_sensor \
  $(SDK_ROOT)/components/libraries/twi_mngr \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/libraries/sensorsim \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/softdevice/$(SOFT_DEVICE)/headers/nrf52 \
  $(SDK_ROOT)/components/softdevice/$(SOFT_DEVICE)/headers \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr \
  $(SDK_ROOT)/components/ble/nrf_ble_scan \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/integration/nrfx/legacy \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/modules/nrfx/mdk \
  $(SDK_ROOT)/modules/nrfx/soc \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/modules/nrfx/drivers/src \
  $(SDK_ROOT)/modules/nrfx/drivers/src/prs \

NRF_PATHS += \
	$(INC_DIRS) \

XS_OBJ = \
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

SDK_GLUE_OBJ = \
	$(TMP_DIR)/xsmain.c.o \
	$(TMP_DIR)/systemclock.c.o \
	$(TMP_DIR)/debugger.c.o \
	$(TMP_DIR)/main.c.o \

SDK_GLUE_DIRS = \
	$(BUILD_DIR)/devices/nrf52/base \

BOARD_SUPPORT = \
	$(LIB_DIR)/boards.c.o \
	$(LIB_DIR)/bsp.c.o \
	$(LIB_DIR)/bsp_btn_ble.c.o \

FREERTOS_OBJECTS = \
	$(LIB_DIR)/croutine.c.o \
	$(LIB_DIR)/event_groups.c.o \
	$(LIB_DIR)/heap_1.c.o \
	$(LIB_DIR)/list.c.o \
	$(LIB_DIR)/port_cmsis_systick.c.o \
	$(LIB_DIR)/port_cmsis.c.o \
	$(LIB_DIR)/port.c.o \
	$(LIB_DIR)/queue.c.o \
	$(LIB_DIR)/stream_buffer.c.o \
	$(LIB_DIR)/tasks.c.o \
	$(LIB_DIR)/timers.c.o

STARTUP_OBJECTS = \
	$(LIB_DIR)/gcc_startup_nrf52840.S.o \
	$(LIB_DIR)/system_nrf52840.c.o \
	$(LIB_DIR)/hardfault_handler_gcc.c.o \
	$(LIB_DIR)/hardfault_implementation.c.o \

NRF_BLE_OBJECTS = \
	$(LIB_DIR)/auth_status_tracker.c.o \
	$(LIB_DIR)/ble_advdata.c.o \
	$(LIB_DIR)/ble_advertising.c.o \
	$(LIB_DIR)/ble_conn_params.c.o \
	$(LIB_DIR)/ble_conn_state.c.o \
	$(LIB_DIR)/ble_srv_common.c.o \
	$(LIB_DIR)/gatt_cache_manager.c.o \
	$(LIB_DIR)/gatts_cache_manager.c.o \
	$(LIB_DIR)/id_manager.c.o \
	$(LIB_DIR)/nrf_ble_gatt.c.o \
	$(LIB_DIR)/nrf_ble_qwr.c.o \
	$(LIB_DIR)/nrf_ble_scan.c.o \
	$(LIB_DIR)/peer_data_storage.c.o \
	$(LIB_DIR)/peer_database.c.o \
	$(LIB_DIR)/peer_id.c.o \
	$(LIB_DIR)/peer_manager_handler.c.o \
	$(LIB_DIR)/peer_manager.c.o \
	$(LIB_DIR)/pm_buffer.c.o \
	$(LIB_DIR)/security_dispatcher.c.o \
	$(LIB_DIR)/security_manager.c.o

NRF_CRYPTO_OBJECTS = \
	$(LIB_DIR)/nrf_crypto_aead.c.o \
	$(LIB_DIR)/nrf_crypto_aes.c.o \
	$(LIB_DIR)/nrf_crypto_aes_shared.c.o \
	$(LIB_DIR)/nrf_crypto_ecc.c.o \
	$(LIB_DIR)/nrf_crypto_ecdh.c.o \
	$(LIB_DIR)/nrf_crypto_ecdsa.c.o \
	$(LIB_DIR)/nrf_crypto_eddsa.c.o \
	$(LIB_DIR)/nrf_crypto_error.c.o \
	$(LIB_DIR)/nrf_crypto_hash.c.o \
	$(LIB_DIR)/nrf_crypto_hkdf.c.o \
	$(LIB_DIR)/nrf_crypto_hmac.c.o \
	$(LIB_DIR)/nrf_crypto_init.c.o \
	$(LIB_DIR)/nrf_crypto_rng.c.o \
	$(LIB_DIR)/nrf_crypto_shared.c.o

NRF_HW_CRYPTO_BACKEND_OBJECTS = \
	$(LIB_DIR)/nrf_hw_backend_init.c.o \
	$(LIB_DIR)/nrf_hw_backend_rng.c.o \
	$(LIB_DIR)/nrf_hw_backend_rng_mbedtls.c.o

NRF_CRYPTO_BACKEND_CC310_OBJECTS = \
	$(LIB_DIR)/cc310_backend_aes.c.o \
	$(LIB_DIR)/cc310_backend_aes_aead.c.o \
	$(LIB_DIR)/cc310_backend_chacha_poly_aead.c.o \
	$(LIB_DIR)/cc310_backend_ecc.c.o \
	$(LIB_DIR)/cc310_backend_ecdh.c.o \
	$(LIB_DIR)/cc310_backend_ecdsa.c.o \
	$(LIB_DIR)/cc310_backend_eddsa.c.o \
	$(LIB_DIR)/cc310_backend_hash.c.o \
	$(LIB_DIR)/cc310_backend_hmac.c.o \
	$(LIB_DIR)/cc310_backend_init.c.o \
	$(LIB_DIR)/cc310_backend_mutex.c.o \
	$(LIB_DIR)/cc310_backend_rng.c.o \
	$(LIB_DIR)/cc310_backend_shared.c.o

NRF_DRIVERS = \
	$(LIB_DIR)/nrf_drv_clock.c.o \
	$(LIB_DIR)/nrf_drv_spi.c.o \
	$(LIB_DIR)/nrf_drv_twi.c.o \
	$(LIB_DIR)/nrf_drv_uart.c.o \
	$(LIB_DIR)/nrfx_atomic.c.o \
	$(LIB_DIR)/nrfx_clock.c.o \
	$(LIB_DIR)/nrfx_gpiote.c.o \
	$(LIB_DIR)/nrfx_lpcomp.c.o \
	$(LIB_DIR)/nrfx_prs.c.o \
	$(LIB_DIR)/nrfx_qdec.c.o \
	$(LIB_DIR)/nrfx_saadc.c.o \
	$(LIB_DIR)/nrfx_spim.c.o \
	$(LIB_DIR)/nrfx_twim.c.o \
	$(LIB_DIR)/nrfx_uart.c.o \
	$(LIB_DIR)/nrfx_uarte.c.o

NRF_LIBRARIES = \
	$(LIB_DIR)/app_button.c.o \
	$(LIB_DIR)/app_error.c.o \
	$(LIB_DIR)/app_error_handler_gcc.c.o \
	$(LIB_DIR)/app_error_weak.c.o \
	$(LIB_DIR)/app_timer_freertos.c.o \
	$(LIB_DIR)/app_util_platform.c.o \
	$(LIB_DIR)/fds.c.o \
	$(LIB_DIR)/nrf_assert.c.o \
	$(LIB_DIR)/nrf_atfifo.c.o \
	$(LIB_DIR)/nrf_atflags.c.o \
	$(LIB_DIR)/nrf_atomic.c.o \
	$(LIB_DIR)/nrf_balloc.c.o \
	$(LIB_DIR)/nrf_fprintf.c.o \
	$(LIB_DIR)/nrf_fprintf_format.c.o \
	$(LIB_DIR)/nrf_fstorage_sd.c.o \
	$(LIB_DIR)/nrf_fstorage.c.o \
	$(LIB_DIR)/nrf_memobj.c.o \
	$(LIB_DIR)/nrf_queue.c.o \
	$(LIB_DIR)/nrf_ringbuf.c.o \
	$(LIB_DIR)/nrf_section_iter.c.o \
	$(LIB_DIR)/nrf_serial.c.o \
	$(LIB_DIR)/nrf_spi_mngr.c.o \
	$(LIB_DIR)/nrf_strerror.c.o \
	$(LIB_DIR)/nrf_twi_mngr.c.o \
	$(LIB_DIR)/nrf_twi_sensor.c.o \
	$(LIB_DIR)/sensorsim.c.o

NRF_LOG_OBJECTS = \
	$(LIB_DIR)/nrf_log_backend_rtt.c.o \
	$(LIB_DIR)/nrf_log_backend_serial.c.o \
	$(LIB_DIR)/nrf_log_backend_uart.c.o \
	$(LIB_DIR)/nrf_log_default_backends.c.o \
	$(LIB_DIR)/nrf_log_frontend.c.o \
	$(LIB_DIR)/nrf_log_str_formatter.c.o

NRF_SOFTDEVICE = \
	$(LIB_DIR)/nrf_sdh_ble.c.o \
	$(LIB_DIR)/nrf_sdh_freertos.c.o \
	$(LIB_DIR)/nrf_sdh_soc.c.o \
	$(LIB_DIR)/nrf_sdh.c.o

OBJECTS += \
	$(BOARD_SUPPORT) \
	$(SEGGER_RTT) \
	$(FREERTOS_OBJECTS) \
	$(STARTUP_OBJECTS) \
	$(NRF_BLE_OBJECTS) \
	$(NRF_CRYPTO_OBJECTS) \
	$(NRF_CRYPTO_BACKEND_CC310_OBJECTS) \
	$(NRF_HW_CRYPTO_BACKEND_OBJECTS) \
	$(NRF_LOG_OBJECTS) \
	$(NRF_DRIVERS) \
	$(NRF_LIBRARIES) \
	$(NRF_SOFTDEVICE)

OTHER_STUFF += \
	boards_h

TOOLS_BIN = $(NRF52_GCC_ROOT)/bin
TOOLS_PREFIX = arm-none-eabi-

CC  = $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc
CPP = $(TOOLS_BIN)/$(TOOLS_PREFIX)g++
#LD  = $(TOOLS_BIN)/$(TOOLS_PREFIX)ld
LD  = $(TOOLS_BIN)/$(TOOLS_PREFIX)gcc
AR  = $(TOOLS_BIN)/$(TOOLS_PREFIX)ar
OBJCOPY = $(TOOLS_BIN)/$(TOOLS_PREFIX)objcopy
SIZE  = $(TOOLS_BIN)/$(TOOLS_PREFIX)size

AR_FLAGS = crs

MODDABLE_TOOLS_DIR = $(BUILD_DIR)/bin/mac/debug
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

NRF_ASM_FLAGS= \
	-mcpu=cortex-m4 \
	-mthumb -mabi=aapcs
	-mlittle-endian \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-D$(BOARD_DEF) \
	-DBSP_DEFINES_ONLY \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DNRF52840_XXAA \


NRF_C_DEFINES= \
	-D__SIZEOF_WCHAR_T=4 \
	-D__ARM_ARCH_7EM__ \
	-D__ARM_ARCH_FPV4_SP_D16__ \
	-D__HEAP_SIZE__=$(HEAP_SIZE) \
	-D__GNU_LINKER \
	-D$(BOARD_DEF) \
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
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION) \

C_FLAGS=\
	-c	\
	-std=gnu99 \
	--sysroot=$(NRF52_GCC_ROOT)/arm-none-eabi \
	-ffunction-sections -fdata-sections -fno-strict-aliasing \
	-fno-short-enums \
	-fno-common \
	-fomit-frame-pointer \
	-fno-dwarf2-cfi-asm \
	-fno-builtin \
	-gdwarf-3 \
	-gpubnames \

ifeq ($(DEBUG),1)
	C_DEFINES += \
		-DDEBUG=1 \
		-DmxDebug=1 \
		-DDEBUG_NRF \
		-g3 \
		-Os
	C_FLAGS += $(HW_DEBUG_OPT)
	ASM_FLAGS += $(HW_DEBUG_OPT) -DDEBUG_NRF
else
	C_DEFINES += \
		-Os \
		-DmxDebug=0
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

C_FLAGS_NODATASECTION = $(C_FLAGS)

# LINKER_SCRIPT := $(PLATFORM_DIR)/config/generic_gcc_nrf52.ld
LINKER_SCRIPT := $(PLATFORM_DIR)/config/xsproj.ld

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

#-----------------
ifeq ($(DEBUG),1)
	ifeq ($(HOST_OS),Darwin)
		KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)
		DO_XSBUG = open -a $(BUILD_DIR)/bin/mac/release/xsbug.app -g
		DO_LAUNCH = serial2xsbug $(DEBUGGER_PORT) $(DEBUGGER_SPEED) 8N1
	else
		KILL_SERIAL_2_XSBUG = $(shell pkill serial2xsbug)
		DO_XSBUG = $(shell nohup $(BUILD_DIR)/bin/lin/release/xsbug > /dev/null 2>&1 &)
		DO_LAUNCH = 
	endif
else
	KILL_SERIAL_2_XSBUG =
	DO_XSBUG =
	DO_LAUNCH = 
endif

#-----------------

VPATH += $(NRF_PATHS) $(SDK_DIRS) $(SDK_GLUE_DIRS) $(XS_DIRS)

.PHONY: all	
.SUFFIXES:
%.d:
.PRECIOUS: %.d %.o

all: $(BLE) $(TMP_DIR) $(LIB_DIR) $(OTHER_STUFF) $(BIN_DIR)/xs_nrf52.hex

clean:
	@echo "# Cleaning tmp and bin for this project"
	@echo "# rm $(TMP_DIR)"
	-rm -rf $(TMP_DIR) 2>/dev/null
	@echo "# rm $(BIN_DIR)"
	-rm -rf $(BIN_DIR) 2>/dev/null

allclean:
	@echo "# Cleaning all nrf52"
	@echo "# rm $(MODDABLE)/build/bin/nrf52"
	-rm -rf $(MODDABLE)/build/bin/nrf52
	@echo "# rm $(MODDABLE)/build/tmp/nrf52"
	-rm -rf $(MODDABLE)/build/tmp/nrf52

flash: all $(BIN_DIR)/xs_nrf52.hex
	@echo Flashing: $(BIN_DIR)/xs_nrf52.hex
	nrfjprog -f nrf52 --program $(BIN_DIR)/xs_nrf52.hex --sectorerase
	nrfjprog -f nrf52 --reset

flash_softdevice:
	@echo Flashing: s140_nrf52_6.1.1_softdevice.hex
	nrfjprog -f nrf52 --program $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex --sectorerase
	nrfjprog -f nrf52 --reset

$(BIN_DIR)/xs_nrf52.uf2: $(BIN_DIR)/xs_nrf52.hex
	@echo Making: $(BIN_DIR)/xs_nrf52.uf2 from xs_nrf52.hex
	$(UF2CONV) $(BIN_DIR)/xs_nrf52.hex -c -f 0xADA52840 -o $(BIN_DIR)/xs_nrf52.uf2

copyToM4: all $(BIN_DIR)/xs_nrf52.uf2
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	@echo Copying: $(BIN_DIR)/xs_nrf52.hex to $(UF2_VOLUME_NAME)
	cp $(BIN_DIR)/xs_nrf52.uf2 /Volumes/$(UF2_VOLUME_NAME)
	$(DO_LAUNCH)

installBootloader:
	nrfjprog --reset --program $(BOOTLOADER_HEX) -f nrf52 --sectoranduicrerase

installSoftdevice:	
	nrfjprog --program $(SOFTDEVICE_HEX) -f nrf52 --chiperase --reset

erase:
	nrfjprog -f nrf52 --eraseall

$(BIN_DIR)/xs_nrf52-merged.hex: $(BOOTLOADER_HEX) $(BIN_DIR)/xs_nrf52.hex
	@echo CR $<
	@mergehex -q -m $(BOOTLOADER_HEX) $(BIN_DIR)/xs_nrf52.hex -o $@

dfu-package: $(BIN_DIR)/xs_nrf52-merged.hex
	@echo "# Packaging $<"
#	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application $< $(BIN_DIR)/dfu-package.zip
#	adafruit-nrfutil dfu genpkg --sd-req 0xB6 --dev-type 0x0052 --application $(BIN_DIR)/xs_nrf52.hex $(BIN_DIR)/dfu-package.zip --bootloader $(BOOTLOADER_HEX) --softdevice $(SOFTDEVICE_HEX)
	adafruit-nrfutil dfu genpkg --sd-req 0xB6 --dev-type 0x0052 --application $(BIN_DIR)/xs_nrf52-merged.hex $(BIN_DIR)/dfu-package.zip

installDFU: all dfu-package
	@echo "# Flashing $<"
	adafruit-nrfutil --verbose dfu serial --package $(BIN_DIR)/dfu-package.zip -p $(NRF_SERIAL_PORT) -b 115200 --singlebank --touch 1200

startDebugger:
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	$(DO_LAUNCH)
	
xall: $(TMP_DIR) $(LIB_DIR) $(BIN_DIR)/xs_nrf52.hex
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	@echo Flashing xs_nrf52.hex to device.
	$(NRFJPROG) -f nrf52 --program $(TMP_DIR)/xs_nrf52.hex --sectorerase
	@echo Resetting the device.
	$(NRFJPROG) -f nrf52 --reset

$(SDK_ROOT)/components/boards/moddable_four.h:
	$(error "## Please add Moddable boards to your NRF52 SDK")
	

boards_h: $(SDK_ROOT)/components/boards/moddable_four.h

$(TMP_DIR):
	@echo "TMP_DIR"
	mkdir -p $(TMP_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" > $(LIB_DIR)/buildinfo.h
	
$(BIN_DIR)/xs_nrf52.bin: $(TMP_DIR)/xs_nrf52.hex
	$(OBJCOPY) -O binary $(TMP_DIR)/xs_nrf52.out $(BIN_DIR)/xs_nrf52.bin

$(BIN_DIR)/xs_nrf52.hex: $(TMP_DIR)/xs_nrf52.out
	@echo "# Size"
	$(SIZE) $(TMP_DIR)/xs_nrf52.out
#	$(OBJCOPY) $(TMP_DIR)/xs_nrf52.out $(BIN_DIR)/xs_nrf52.hex
	$(OBJCOPY) -O ihex $< $@

FINAL_LINK_OBJ:=\
	$(XS_OBJ) \
	$(SDK_GLUE_OBJ) \
	$(SDK_OBJ) \
	$(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o \
	$(OBJECTS) \
	$(LIB_DIR)/buildinfo.c.o

ekoFiles = $(foreach fil,$(FINAL_LINK_OBJ),$(shell echo '$(strip $(fil))' >> $(BIN_DIR)/xs_nrf52.ind1))

$(BIN_DIR)/xs_nrf52.ind: $(FINAL_LINK_OBJ)
	@echo "# creating xs_nrf52.ind"
#	 @echo "# FINAL LINK OBJ: $(FINAL_LINK_OBJ)"
	@rm -f $(BIN_DIR)/xs_nrf52.ind
#	@echo $(ekoFiles)
	$(ekoFiles)
	@mv $(BIN_DIR)/xs_nrf52.ind1 $(BIN_DIR)/xs_nrf52.ind

$(TMP_DIR)/xs_nrf52.out: $(FINAL_LINK_OBJ)
	@echo "# creating xs_nrf52.out"
#	 @echo "# FINAL LINK OBJ: $(FINAL_LINK_OBJ)"
	@rm -f $(TMP_DIR)/xs_nrf52.out
	@echo "# $(LD) $(LDFLAGS) $(FINAL_LINK_OBJ) $(LIB_FILES) -o $@"
	@echo "# Link to .out file"
	$(LD) $(LDFLAGS) $(FINAL_LINK_OBJ) $(LIB_FILES) -o $@

$(LIB_DIR)/buildinfo.c.o: $(SDK_GLUE_OBJ) $(XS_OBJ) $(SDK_OBJ) $(TMP_DIR)/mc.xs.c.o $(TMP_DIR)/mc.resources.c.o $(OBJECTS)
	@echo "# buildinfo"
	echo '#include "buildinfo.h"' > $(LIB_DIR)/buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)/buildinfo.c
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $(LIB_DIR)/buildinfo.c -o $@

$(XS_OBJ): $(XS_HEADERS)
$(LIB_DIR)/xs%.c.o: xs%.c
	@echo "# library xs:" $(<F) "(strings in flash)"
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

