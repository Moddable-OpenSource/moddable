#
# Copyright (c) 2016-2020  Moddable Tech, Inc.
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

HOST_OS = win

!IF "$(NRF_ROOT)"==""
NRF_ROOT = $(USERPROFILE)\nrf5
!ENDIF

!IF "$(NRF52_GNU_VERSION)"==""
NRF52_GNU_VERSION = 8.2.1
!ENDIF

!IF "$(NRF_ROOT)"==""
NRF_ROOT = $(USERPROFILE)\nrf5
!ENDIF

!IF "$(NRF52_GCC_ROOT)"==""
NRF52_GCC_ROOT = $(NRF_ROOT)\gcc-arm-none-eabi-8-2018-q4-major
!ENDIF

!IF "$(NRF_SDK_DIR)"==""
NRF_SDK_DIR = $(NRF_ROOT)\nRF5_SDK
!ENDIF
SDK_ROOT = $(NRF_SDK_DIR)

!IF "$(UF2CONV)"==""
UF2CONV = $(NRF_ROOT)\uf2conv.py
!ENDIF

!IF [exist $(SDK_ROOT)\components\boards\moddable_four.h] == 0
!ERROR ## Please add Moddable boards to your nRF52 SDK
!ENDIF

PLATFORM_DIR = $(MODDABLE)\build\devices\nrf52
DO_COPY =
MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\release
UF2_VOLUME_PATH =
WAIT_FOR_M4 =

!IF "$(DEBUG)"=="1"
DO_XSBUG = tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(MODDABLE_TOOLS_DIR)\xsbug.exe)
KILL_SERIAL_2_XSBUG =-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
WAIT_FOR_NEW_SERIAL =1
!ELSE
DO_XSBUG =
KILL_SERIAL_2_XSBUG =
WAIT_FOR_NEW_SERIAL =
!ENDIF

# nRF52840_xxAA
BOARD = pca10056
BOARD_DEF = BOARD_MODDABLE_FOUR
HEAP_SIZE = 0x13000
HWCPU = cortex-m4
SOFT_DEVICE = s140

!IF "$(DEBUG)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\debug\lib
!ELSEIF "$(INSTRUMENT)"=="1"
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\instrument\lib
!ELSE
LIB_DIR = $(BUILD_DIR)\tmp\$(PLATFORMPATH)\release\lib
!ENDIF

CRYPTO_DIRS = \
	-I$(SDK_ROOT)\components\libraries\crypto \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\cc310 \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\cc310_bl \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\cifra \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\nrf_hw \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\mbedtls \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\micro_ecc \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\nrf_sw \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\oberon \
	-I$(SDK_ROOT)\components\libraries\crypto\backend\optiga \
	-I$(SDK_ROOT)\external\nrf_cc310\include \
	-I$(SDK_ROOT)\external\nrf_cc310_bl\include

FREE_RTOS_DIRS = \
	-I$(SDK_ROOT)\external\freertos\source \
	-I$(SDK_ROOT)\external\freertos\source\include \
	-I$(SDK_ROOT)\external\freertos\source\portable\MemMang \
	-I$(SDK_ROOT)\external\freertos\portable\GCC\nrf52 \
	-I$(SDK_ROOT)\external\freertos\portable\CMSIS\nrf52

GCC_DIRS = \
	-I$(NRF52_GCC_ROOT)\arm-none-eabi\include \
	-I$(NRF52_GCC_ROOT)\arm-none-eabi\include\machine \
	-I$(NRF52_GCC_ROOT)\lib\gcc\arm-none-eabi\$(NRF52_GNU_VERSION)\include \
	-I$(NRF52_GCC_ROOT)\lib\gcc\arm-none-eabi\$(NRF52_GNU_VERSION)\include-fixed

SDK_DIRS = \
	-I$(SDK_ROOT)\components \
	-I$(SDK_ROOT)\components\boards \
	-I$(SDK_ROOT)\components\libraries\atomic \
	-I$(SDK_ROOT)\components\libraries\atomic_fifo \
	-I$(SDK_ROOT)\components\libraries\atomic_flags \
	-I$(SDK_ROOT)\components\libraries\balloc \
	-I$(SDK_ROOT)\components\libraries\button \
	-I$(SDK_ROOT)\components\libraries\bsp \
	-I$(SDK_ROOT)\components\libraries\delay \
	-I$(SDK_ROOT)\components\libraries\fds \
	-I$(SDK_ROOT)\components\libraries\fstorage \
	-I$(SDK_ROOT)\components\libraries\hardfault \
	-I$(SDK_ROOT)\components\libraries\hardfault\nrf52 \
	-I$(SDK_ROOT)\components\libraries\hardfault\nrf52\handler \
	-I$(SDK_ROOT)\components\libraries\log \
	-I$(SDK_ROOT)\components\libraries\log\src \
	-I$(SDK_ROOT)\components\libraries\queue \
	-I$(SDK_ROOT)\components\libraries\ringbuf \
	-I$(SDK_ROOT)\components\libraries\scheduler \
	-I$(SDK_ROOT)\components\libraries\serial \
	-I$(SDK_ROOT)\components\libraries\spi_mngr \
	-I$(SDK_ROOT)\components\libraries\stack_info \
	-I$(SDK_ROOT)\components\libraries\strerror \
	-I$(SDK_ROOT)\components\libraries\twi_sensor \
	-I$(SDK_ROOT)\components\libraries\twi_mngr \
	-I$(SDK_ROOT)\components\libraries\timer \
	-I$(SDK_ROOT)\components\libraries\util \
	-I$(SDK_ROOT)\components\libraries\experimental_section_vars \
	-I$(SDK_ROOT)\components\libraries\mutex \
	-I$(SDK_ROOT)\components\libraries\memobj \
	-I$(SDK_ROOT)\components\libraries\log\src \
	-I$(SDK_ROOT)\components\libraries\sensorsim \
	-I$(SDK_ROOT)\components\libraries\usbd \
	-I$(SDK_ROOT)\components\libraries\usbd\class\cdc \
	-I$(SDK_ROOT)\components\libraries\usbd\class\cdc\acm \
	-I$(SDK_ROOT)\components\toolchain\cmsis\include \
	-I$(SDK_ROOT)\components\softdevice\$(SOFT_DEVICE)\headers\nrf52 \
	-I$(SDK_ROOT)\components\softdevice\$(SOFT_DEVICE)\headers \
	-I$(SDK_ROOT)\components\softdevice\common \
	-I$(SDK_ROOT)\components\ble\common \
	-I$(SDK_ROOT)\components\ble\ble_advertising \
	-I$(SDK_ROOT)\components\ble\nrf_ble_gatt \
	-I$(SDK_ROOT)\components\ble\nrf_ble_qwr \
	-I$(SDK_ROOT)\components\ble\nrf_ble_scan \
	-I$(SDK_ROOT)\components\ble\peer_manager \
	-I$(SDK_ROOT)\external\fprintf \
	-I$(SDK_ROOT)\external\utf_converter \
	-I$(SDK_ROOT)\integration\nrfx\legacy \
	-I$(SDK_ROOT)\integration\nrfx \
	-I$(SDK_ROOT)\modules\nrfx \
	-I$(SDK_ROOT)\modules\nrfx\hal \
	-I$(SDK_ROOT)\modules\nrfx\mdk \
	-I$(SDK_ROOT)\modules\nrfx\soc \
	-I$(SDK_ROOT)\modules\nrfx\drivers\include \
	-I$(SDK_ROOT)\modules\nrfx\drivers\src \
	-I$(SDK_ROOT)\modules\nrfx\drivers\src\prs

SDK_GLUE_DIRS = \
	-I$(BUILD_DIR)\devices\nrf52\base

XS_DIRS = \
	-I$(XS_DIR)\includes \
	-I$(XS_DIR)\sources \
	-I$(XS_DIR)\platforms\nrf52 \
	-I$(BUILD_DIR)\devices\nrf52

BOARD_SUPPORT_OBJECTS = \
	$(LIB_DIR)\boards.o \
	$(LIB_DIR)\bsp.o \
	$(LIB_DIR)\bsp_btn_ble.o

FREERTOS_OBJECTS = \
	$(LIB_DIR)\croutine.o \
	$(LIB_DIR)\event_groups.o \
	$(LIB_DIR)\heap_1.o \
	$(LIB_DIR)\list.o \
	$(LIB_DIR)\port_cmsis_systick.o \
	$(LIB_DIR)\port_cmsis.o \
	$(LIB_DIR)\port.o \
	$(LIB_DIR)\queue.o \
	$(LIB_DIR)\stream_buffer.o \
	$(LIB_DIR)\tasks.o \
	$(LIB_DIR)\timers.o

NRF_BLE_OBJECTS = \
	$(LIB_DIR)\auth_status_tracker.o \
	$(LIB_DIR)\ble_advdata.o \
	$(LIB_DIR)\ble_advertising.o \
	$(LIB_DIR)\ble_conn_params.o \
	$(LIB_DIR)\ble_conn_state.o \
	$(LIB_DIR)\ble_srv_common.o \
	$(LIB_DIR)\gatt_cache_manager.o \
	$(LIB_DIR)\gatts_cache_manager.o \
	$(LIB_DIR)\id_manager.o \
	$(LIB_DIR)\nrf_ble_gatt.o \
	$(LIB_DIR)\nrf_ble_qwr.o \
	$(LIB_DIR)\nrf_ble_scan.o \
	$(LIB_DIR)\peer_data_storage.o \
	$(LIB_DIR)\peer_database.o \
	$(LIB_DIR)\peer_id.o \
	$(LIB_DIR)\peer_manager_handler.o \
	$(LIB_DIR)\peer_manager.o \
	$(LIB_DIR)\pm_buffer.o \
	$(LIB_DIR)\security_dispatcher.o \
	$(LIB_DIR)\security_manager.o

NRF_CRYPTO_OBJECTS = \
	$(LIB_DIR)\nrf_crypto_aead.o \
	$(LIB_DIR)\nrf_crypto_aes.o \
	$(LIB_DIR)\nrf_crypto_aes_shared.o \
	$(LIB_DIR)\nrf_crypto_ecc.o \
	$(LIB_DIR)\nrf_crypto_ecdh.o \
	$(LIB_DIR)\nrf_crypto_ecdsa.o \
	$(LIB_DIR)\nrf_crypto_eddsa.o \
	$(LIB_DIR)\nrf_crypto_error.o \
	$(LIB_DIR)\nrf_crypto_hash.o \
	$(LIB_DIR)\nrf_crypto_hkdf.o \
	$(LIB_DIR)\nrf_crypto_hmac.o \
	$(LIB_DIR)\nrf_crypto_init.o \
	$(LIB_DIR)\nrf_crypto_rng.o \
	$(LIB_DIR)\nrf_crypto_shared.o

NRF_DRIVERS_OBJECTS = \
	$(LIB_DIR)\nrf_drv_clock.o \
	$(LIB_DIR)\nrf_drv_power.o \
	$(LIB_DIR)\nrf_drv_spi.o \
	$(LIB_DIR)\nrf_drv_twi.o \
	$(LIB_DIR)\nrf_drv_uart.o \
	$(LIB_DIR)\nrfx_atomic.o \
	$(LIB_DIR)\nrfx_clock.o \
	$(LIB_DIR)\nrfx_gpiote.o \
	$(LIB_DIR)\nrfx_lpcomp.o \
	$(LIB_DIR)\nrfx_power.o \
	$(LIB_DIR)\nrfx_prs.o \
	$(LIB_DIR)\nrfx_qdec.o \
	$(LIB_DIR)\nrfx_saadc.o \
	$(LIB_DIR)\nrfx_spim.o \
	$(LIB_DIR)\nrfx_systick.o \
	$(LIB_DIR)\nrfx_twim.o \
	$(LIB_DIR)\nrfx_uart.o \
	$(LIB_DIR)\nrfx_uarte.o \
	$(LIB_DIR)\nrfx_wdt.o

NRF_CRYPTO_BACKEND_CC310_OBJECTS = \
	$(LIB_DIR)\cc310_backend_aes.o \
	$(LIB_DIR)\cc310_backend_aes_aead.o \
	$(LIB_DIR)\cc310_backend_chacha_poly_aead.o \
	$(LIB_DIR)\cc310_backend_ecc.o \
	$(LIB_DIR)\cc310_backend_ecdh.o \
	$(LIB_DIR)\cc310_backend_ecdsa.o \
	$(LIB_DIR)\cc310_backend_eddsa.o \
	$(LIB_DIR)\cc310_backend_hash.o \
	$(LIB_DIR)\cc310_backend_hmac.o \
	$(LIB_DIR)\cc310_backend_init.o \
	$(LIB_DIR)\cc310_backend_mutex.o \
	$(LIB_DIR)\cc310_backend_rng.o \
	$(LIB_DIR)\cc310_backend_shared.o

NRF_HW_CRYPTO_BACKEND_OBJECTS = \
	$(LIB_DIR)\nrf_hw_backend_init.o \
	$(LIB_DIR)\nrf_hw_backend_rng.o \
	$(LIB_DIR)\nrf_hw_backend_rng_mbedtls.o

NRF_LIBRARIES_OBJECTS = \
	$(LIB_DIR)\app_button.o \
	$(LIB_DIR)\app_error.o \
	$(LIB_DIR)\app_error_handler_gcc.o \
	$(LIB_DIR)\app_error_weak.o \
	$(LIB_DIR)\app_timer_freertos.o \
	$(LIB_DIR)\app_util_platform.o \
	$(LIB_DIR)\fds.o \
	$(LIB_DIR)\nrf_assert.o \
	$(LIB_DIR)\nrf_atfifo.o \
	$(LIB_DIR)\nrf_atflags.o \
	$(LIB_DIR)\nrf_atomic.o \
	$(LIB_DIR)\nrf_balloc.o \
	$(LIB_DIR)\nrf_fprintf.o \
	$(LIB_DIR)\nrf_fprintf_format.o \
	$(LIB_DIR)\nrf_fstorage_sd.o \
	$(LIB_DIR)\nrf_fstorage.o \
	$(LIB_DIR)\nrf_memobj.o \
	$(LIB_DIR)\nrf_queue.o \
	$(LIB_DIR)\nrf_ringbuf.o \
	$(LIB_DIR)\nrf_section_iter.o \
	$(LIB_DIR)\nrf_serial.o \
	$(LIB_DIR)\nrf_spi_mngr.o \
	$(LIB_DIR)\nrf_strerror.o \
	$(LIB_DIR)\nrf_twi_mngr.o \
	$(LIB_DIR)\nrf_twi_sensor.o \
	$(LIB_DIR)\sensorsim.o

NRF_LOG_OBJECTS = \
	$(LIB_DIR)\nrf_log_backend_rtt.o \
	$(LIB_DIR)\nrf_log_backend_serial.o \
	$(LIB_DIR)\nrf_log_backend_uart.o \
	$(LIB_DIR)\nrf_log_default_backends.o \
	$(LIB_DIR)\nrf_log_frontend.o \
	$(LIB_DIR)\nrf_log_str_formatter.o

NRF_SOFTDEVICE_OBJECTS = \
	$(LIB_DIR)\nrf_sdh_ble.o \
	$(LIB_DIR)\nrf_sdh_freertos.o \
	$(LIB_DIR)\nrf_sdh_soc.o \
	$(LIB_DIR)\nrf_sdh.o

NRF_USBD_OBJECTS = \
	$(LIB_DIR)\app_usbd.o \
	$(LIB_DIR)\app_usbd_cdc_acm.o \
	$(LIB_DIR)\app_usbd_core.o \
	$(LIB_DIR)\app_usbd_serial_num.o \
	$(LIB_DIR)\app_usbd_string_desc.o \
	$(LIB_DIR)\nrfx_usbd.o

SDK_GLUE_OBJECTS = \
	$(TMP_DIR)\xsmain.o \
	$(TMP_DIR)\systemclock.o \
	$(TMP_DIR)\debugger.o \
	$(TMP_DIR)\ftdi_trace.o \
	$(TMP_DIR)\main.o \
	$(TMP_DIR)\debugger_usbd.o

STARTUP_OBJECTS = \
	$(LIB_DIR)\gcc_startup_nrf52840.S.o \
	$(LIB_DIR)\system_nrf52840.o \
	$(LIB_DIR)\hardfault_handler_gcc.o \
	$(LIB_DIR)\hardfault_implementation.o

XS_OBJ = \
	$(LIB_DIR)\xsHost.o \
	$(LIB_DIR)\xsPlatform.o \
	$(LIB_DIR)\xsAll.o \
	$(LIB_DIR)\xsAPI.o \
	$(LIB_DIR)\xsArguments.o \
	$(LIB_DIR)\xsArray.o \
	$(LIB_DIR)\xsAtomics.o \
	$(LIB_DIR)\xsBigInt.o \
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
	$(LIB_DIR)\xsre.o

OBJECTS = \
	$(BOARD_SUPPORT_OBJECTS) \
	$(FREERTOS_OBJECTS) \
	$(STARTUP_OBJECTS) \
	$(NRF_BLE_OBJECTS) \
	$(NRF_CRYPTO_OBJECTS) \
	$(NRF_CRYPTO_BACKEND_CC310_OBJECTS) \
	$(NRF_HW_CRYPTO_BACKEND_OBJECTS) \
	$(NRF_LOG_OBJECTS) \
	$(NRF_DRIVERS_OBJECTS) \
	$(NRF_LIBRARIES_OBJECTS) \
	$(NRF_SOFTDEVICE_OBJECTS) \
	$(NRF_USBD_OBJECTS)

FINAL_LINK_OBJ = \
	$(XS_OBJ) \
	$(SDK_GLUE_OBJECTS) \
	$(TMP_DIR)\mc.xs.o
	$(TMP_DIR)\mc.resources.o \
	$(OBJECTS) \
	$(LIB_DIR)\buildinfo.o

XS_HEADERS = \
	$(XS_DIR)\includes\xs.h \
	$(XS_DIR)\includes\xsmc.h \
	$(XS_DIR)\sources\xsAll.h \
	$(XS_DIR)\sources\xsCommon.h \
	$(XS_DIR)\platforms\nrf52\xsPlatform.h \
	$(XS_DIR)\platforms\nrf52\xsHost.h \
	$(BUILD_DIR)\devices\nrf52\config\sdk_config.h

HEADERS = $(HEADERS) $(XS_HEADERS)

LIB_FILES = \
	$(SDK_ROOT)\external\nrf_cc310\lib\cortex-m4\hard-float\no-interrupts\libnrf_cc310_0.9.12.a

TOOLS_BIN = $(NRF52_GCC_ROOT)\bin
CC  = $(TOOLS_BIN)\arm-none-eabi-gcc
CPP = $(TOOLS_BIN)\arm-none-eabi-g++
LD  = $(TOOLS_BIN)\arm-none-eabi-gcc
AR  = $(TOOLS_BIN)\arm-none-eabi-ar
OBJCOPY = $(TOOLS_BIN)\arm-none-eabi-objcopy
SIZE  = $(TOOLS_BIN)\arm-none-eabi-size

BUILDCLUT = $(MODDABLE_TOOLS_DIR)\buildclut
COMPRESSBMF = $(MODDABLE_TOOLS_DIR)\compressbmf
RLE4ENCODE = $(MODDABLE_TOOLS_DIR)\rle4encode
MCLOCAL = $(MODDABLE_TOOLS_DIR)\mclocal
MCREZ = $(MODDABLE_TOOLS_DIR)\mcrez
PNG2BMP = $(MODDABLE_TOOLS_DIR)\png2bmp
IMAGE2CS = $(MODDABLE_TOOLS_DIR)\image2cs
WAV2MAUD = $(MODDABLE_TOOLS_DIR)\wav2maud
BLES2GATT = $(MODDABLE_TOOLS_DIR)\bles2gatt
XSC = $(MODDABLE_TOOLS_DIR)\xsc
XSID = $(MODDABLE_TOOLS_DIR)\xsid
XSL = $(MODDABLE_TOOLS_DIR)\xsl

NRF_C_DEFINES = \
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
	-Dnrf52

C_DEFINES = \
	$(NRF_C_DEFINES) \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(DISPLAY) \
	-DkPocoRotation=$(ROTATION) \
	-DMODGCC=1 \
	-DUSE_FTDI_TRACE=0 \
	-fshort-enums
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) -DMODINSTRUMENTATION=1 -DmxInstrument=1
!ENDIF

C_FLAGS = \
	-c \
	-std=gnu99 \
	--sysroot=$(NRF52_GCC_ROOT)\arm-none-eabi \
	-fdata-sections \
	-ffunction-sections \
	-fmessage-length=0 \
	-fno-builtin \
	-fno-common \
	-fno-diagnostics-show-caret \
	-fno-dwarf2-cfi-asm \
	-fno-strict-aliasing \
	-fomit-frame-pointer \
	-fshort-enums \
	-gdwarf-3 \
	-gpubnames \
	-mcpu=$(HWCPU) \
	-mfloat-abi=hard \
	-mlittle-endian \
	-mfpu=fpv4-sp-d16 \
	-mthumb	\
	-mthumb-interwork \
	-mtp=soft \
	-munaligned-access \
	-nostdinc

C_FLAGS_NODATASECTION = $(C_FLAGS)

ASMFLAGS = \
	$(ASMFLAGS) \
	-mcpu=cortex-m4 \
	-mthumb \
	-mabi=aapcs \
	-D$(BOARD_DEF) \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DNRF52840_XXAA \
	$(FP_OPTS) \
	-DFREERTOS \
	-DNRF_SD_BLE_API_VERSION=6 \
	-DS140 \
	-DSOFTDEVICE_PRESENT

LDFLAGS = \
	-mabi=aapcs \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-mthumb \
	-no-enum-size-warning \
	-Map=$(BIN_DIR)\xs_lib.map \
	-L$(SDK_ROOT)\modules\nrfx\mdk \
	-T$(LINKER_SCRIPT) \
	-Wl,--gc-sections \
	-Xlinker \
	--specs=nano.specs

HW_DEBUG_OPT = $(FP_OPTS)
HW_OPT = -O2 $(FP_OPTS)

!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1 -DDEBUG=1 -DDEBUG_NRF DUSE_DEBUGGER_USBD=1 -g3 -Os
C_FLAGS = $(C_FLAGS) $(HW_DEBUG_OPT)
ASM_FLAGS = $(ASM_FLAGS) $(HW_DEBUG_OPT) -DDEBUG_NRF
!ELSE
C_DEFINES = $(C_DEFINES) -Os
C_FLAGS = $(C_FLAGS) $(HW_OPT)
ASM_FLAGS = $(ASM_FLAGS) $(HW_OPT)
!ENDIF

C_INCLUDES = $(C_INCLUDES) $(GCC_DIRS) $(CRYPTO_DIRS) $(SDK_DIRS) $(PLATFORM_DIR) $(FREE_RTOS_DIRS) $(SDK_GLUE_DIRS) $(XS_DIRS) -I$(LIB_DIR) -I$(TMP_DIR)

LINKER_SCRIPT = $(PLATFORM_DIR)\config\xsproj.ld

.PHONY: all	

precursor: $(BLE) $(TMP_DIR) $(LIB_DIR) $(BIN_DIR)\xs_nrf52.hex

all: precursor $(BIN_DIR)\xs_nrf52.uf2
	$(WAIT_FOR_M4)
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	@echo Copying: $(BIN_DIR)\xs_nrf52.hex to $(UF2_VOLUME_NAME)
	$(DO_COPY)
	$(WAIT_FOR_NEW_SERIAL)

clean:
	echo # Clean project
	echo $(BIN_DIR)
	del /s/q/f $(BIN_DIR)\*.* > NUL
	rmdir /s/q $(BIN_DIR)
	echo $(TMP_DIR)
	del /s/q/f $(TMP_DIR)\*.* > NUL
	rmdir /s/q $(TMP_DIR)
	echo $(LIB_DIR)
	if exist $(LIB_DIR) del /s/q/f $(LIB_DIR)\*.* > NUL
	if exist $(LIB_DIR) rmdir /s/q $(LIB_DIR)

xsbug:
	$(WAIT_FOR_M4)
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	$(WAIT_FOR_NEW_SERIAL)

$(BIN_DIR)\xs_nrf52.uf2: $(BIN_DIR)\xs_nrf52.hex
	@echo Making: $(BIN_DIR)\xs_nrf52.uf2 from xs_nrf52.hex
	$(UF2CONV) $(BIN_DIR)\xs_nrf52.hex -c -f 0xADA52840 -o $(BIN_DIR)\xs_nrf52.uf2

$(TMP_DIR):
	if not exist $(TMP_DIR)\$(NULL) mkdir $(TMP_DIR)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h
	
$(BIN_DIR)\xs_nrf52.bin: $(TMP_DIR)\xs_nrf52.hex
	$(OBJCOPY) -O binary $(TMP_DIR)\xs_nrf52.out $(BIN_DIR)\xs_nrf52.bin

$(BIN_DIR)\xs_nrf52.hex: $(TMP_DIR)\xs_nrf52.out
	$(SIZE) -A $(TMP_DIR)\xs_nrf52.out | perl -e $(MEM_USAGE)
	$(OBJCOPY) -O ihex $< $@

$(TMP_DIR)\xs_nrf52.out: $(FINAL_LINK_OBJ)
	@echo # creating xs_nrf52.out
	if exist $(TMP_DIR)\xs_nrf52.out del /s/q/f $(TMP_DIR)\xs_nrf52.out > NUL
	@echo # Link to .out file
	$(LD) $(LDFLAGS) $(FINAL_LINK_OBJ) $(LIB_FILES) -lc -lnosys -lm -o $@

$(LIB_DIR)\buildinfo.o: $(SDK_GLUE_OBJECTS) $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS)
	@echo # buildinfo
	echo '#include "buildinfo.h"' > $(LIB_DIR)\buildinfo.c
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $(LIB_DIR)\buildinfo.c -o $@

$(XS_OBJ): $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # library xs: $(@F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

{$(LIB_DIR)\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@

$(LIB_DIR)/%.S.o %.s.o: %.S
	@echo # asm $(@F)
	$(CC) -c -x assembler-with-cpp $(ASMFLAGS) $(C_INCLUDES) $< -o $@

$(TMP_DIR)/%.o: %.c
	@echo # application: $(@F))
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $< -o $@

$(TMP_DIR)/mc.%.o: $(TMP_DIR)/mc.%.c
	@echo # cc $(@F)
	$(CC) $< $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS_NODATASECTION) -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)

$(TMP_DIR)\mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -p nrf52 -r mc.resources.c
