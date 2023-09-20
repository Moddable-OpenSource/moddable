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

HOST_OS = win

!IF "$(NRF52_SDK_PATH)"==""
!ERROR NRF52_SDK_PATH environment variable must be defined! See https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/moddable-four.md for details.
!ENDIF

!IF !EXIST($(NRF52_SDK_PATH)\components\boards\moddable_four.h)
!ERROR Please add moddable_four.h to your nRF52 SDK! See https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/moddable-four.md for details. 
!ENDIF

!IF "$(NRF_ROOT)"==""
NRF_ROOT = $(USERPROFILE)\nrf5
!ENDIF

!IF "$(NRF52_GCC_ROOT)"==""
NRF52_GCC_ROOT = $(NRF_ROOT)\arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-arm-none-eabi
!ENDIF
!IF "$(NRF52_GNU_VERSION)"==""
NRF52_GNU_VERSION = 12.2.1
!ENDIF

NRF52_SDK_ROOT = $(NRF52_SDK_PATH)


!IF "$(M4_VID)"==""
M4_VID = BEEF
!ENDIF

!IF "$(M4_PID)"==""
M4_PID = CAFE
!ENDIF

NRFJPROG = "c:\Program Files\Nordic Semiconductor\nrf-command-line-tools\bin\nrfjprog"
UF2CONV = $(NRF_ROOT)\uf2conv.py

!IF "$(USE_USB)"==""
USE_USB = 0
!ENDIF

!IF "$(UPLOAD_SPEED)"==""
UPLOAD_SPEED = 921600
!ENDIF
!IF "$(DEBUGGER_SPEED)"==""
DEBUGGER_SPEED = 921600
!ENDIF

#VERBOSE = 1

!IF "$(VERBOSE)"=="1"
!CMDSWITCHES -S
!ELSE
!CMDSWITCHES +S
!ENDIF

MODDABLE_TOOLS_DIR = $(BUILD_DIR)\bin\win\release
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

TOOLS_BIN = $(NRF52_GCC_ROOT)\bin
CC = $(TOOLS_BIN)\arm-none-eabi-gcc
CPP = $(TOOLS_BIN)\arm-none-eabi-g++
LD = $(TOOLS_BIN)\arm-none-eabi-gcc
AR = $(TOOLS_BIN)\arm-none-eabi-ar
OBJCOPY = $(TOOLS_BIN)\arm-none-eabi-objcopy
SIZE = $(TOOLS_BIN)\arm-none-eabi-size

PLATFORM_DIR = $(MODDABLE)\build\devices\nrf52
!IF "$(UF2_VOLUME_NAME)"==""
UF2_VOLUME_NAME = MODDABLE4
!ENDIF
WAIT_FOR_M4 = $(PLATFORM_DIR)\config\waitForVolumeWindows.bat $(UF2_VOLUME_NAME) $(TMP_DIR)\_drive.tmp $(TMP_DIR)\_port.tmp $(M4_VID) $(M4_PID)
DO_COPY = -for /F "tokens=1" %%i in ( $(TMP_DIR)\_drive.tmp ) do @copy $(BIN_DIR)\xs_nrf52.uf2 %%i
ECHO_GIT_AND_SIZE = $(PLATFORM_DIR)\config\echoGitTagAndSizeWindows.bat $(TMP_DIR)\_size.tmp $(MODDABLE) $(NRF52_HEAP_SIZE)

!IF "$(DEBUG)"=="1"
KILL_SERIAL_2_XSBUG =-tasklist /nh /fi "imagename eq serial2xsbug.exe" | (find /i "serial2xsbug.exe" > nul) && taskkill /f /t /im "serial2xsbug.exe" >nul 2>&1
WAIT_FOR_NEW_SERIAL = $(PLATFORM_DIR)\config\waitForNewSerialWindows.bat 1 $(UF2_VOLUME_NAME) $(TMP_DIR)\_port.tmp $(M4_VID) $(M4_PID)
NORESTART = 
!IF "$(XSBUG_LOG)"=="1"
DO_XSBUG =
SERIAL_2_XSBUG = echo Starting serial2xsbug. Type Ctrl-C twice after debugging app. && cd $(MODDABLE)\tools\xsbug-log && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && node xsbug-log start /B $(MODDABLE_TOOLS_DIR)\serial2xsbug $(M4_VID):$(M4_PID) $(DEBUGGER_SPEED) 8N1
!ELSE
DO_XSBUG = tasklist /nh /fi "imagename eq xsbug.exe" | find /i "xsbug.exe" > nul || (start $(MODDABLE_TOOLS_DIR)\xsbug.exe)
SERIAL_2_XSBUG = echo Starting serial2xsbug. Type Ctrl-C twice after debugging app. && set "XSBUG_PORT=$(XSBUG_PORT)" && set "XSBUG_HOST=$(XSBUG_HOST)" && $(MODDABLE_TOOLS_DIR)\serial2xsbug $(M4_VID):$(M4_PID) $(DEBUGGER_SPEED) 8N1 -dtr
!ENDIF
!ELSE
DO_XSBUG =
KILL_SERIAL_2_XSBUG =
WAIT_FOR_NEW_SERIAL = $(PLATFORM_DIR)\config\waitForNewSerialWindows.bat 0 $(UF2_VOLUME_NAME) $(TMP_DIR)\_port.tmp $(M4_VID) $(M4_PID)
SERIAL_2_XSBUG = 
NORESTART =
!ENDIF

!IF "$(FTDI_TRACE)"==""
FTDI_TRACE = -DUSE_FTDI_TRACE=0
!ENDIF

# nRF52840_xxAA
BOARD = pca10056
BOARD_DEF = BOARD_MODDABLE_FOUR

!IF "$(NRF52_HEAP_SIZE)"==""
NRF52_HEAP_SIZE = 0x35000
!ENDIF

HWCPU = cortex-m4
SOFT_DEVICE = s140

!IF "$(DEBUG)"=="1"
!IF "$(USE_USB)"=="1"
DEBUGGER_USBD = -DUSE_DEBUGGER_USBD=1
!ELSE
DEBUGGER_USBD = -DUSE_DEBUGGER_USBD=0
!ENDIF
!ELSE
DEBUGGER_USBD = -DUSE_DEBUGGER_USBD=0
!ENDIF

!IF "$(USE_QSPI)"=="1"
LINKER_SCRIPT = $(PLATFORM_DIR)\config\qspi_xsproj.ld
!ELSE
LINKER_SCRIPT = $(PLATFORM_DIR)\config\xsproj.ld
!ENDIF

GCC_INCLUDES=-iprefix $(NRF52_GCC_ROOT)\ \
	-iwithprefix arm-none-eabi\include \
	-iwithprefix arm-none-eabi\include\machine \
	-iwithprefix lib\gcc\arm-none-eabi\$(NRF52_GNU_VERSION)\include \
	-iwithprefix lib\gcc\arm-none-eabi\$(NRF52_GNU_VERSION)\include-fixed


NRF_SDK_INCLUDES=-iprefix $(NRF52_SDK_ROOT)\components\libraries\ \
	-iwithprefix crypto \
	-iwithprefix crypto\backend\cc310 \
	-iwithprefix crypto\backend\cc310_bl \
	-iwithprefix crypto\backend\cifra \
	-iwithprefix crypto\backend\nrf_hw \
	-iwithprefix crypto\backend\mbedtls \
	-iwithprefix crypto\backend\micro_ecc \
	-iwithprefix crypto\backend\nrf_sw \
	-iwithprefix crypto\backend\oberon \
	-iwithprefix crypto\backend\optiga \
	-iwithprefix atomic \
	-iwithprefix atomic_fifo \
	-iwithprefix atomic_flags \
	-iwithprefix balloc \
	-iwithprefix button \
	-iwithprefix bsp \
	-iwithprefix delay \
	-iwithprefix experimental_section_vars \
	-iwithprefix fds \
	-iwithprefix fifo \
	-iwithprefix fstorage \
	-iwithprefix hardfault \
	-iwithprefix hardfault\nrf52 \
	-iwithprefix hardfault\nrf52\handler \
	-iwithprefix libuarte \
	-iwithprefix log \
	-iwithprefix log\src \
	-iwithprefix memobj \
	-iwithprefix mutex \
	-iwithprefix queue \
	-iwithprefix ringbuf \
	-iwithprefix scheduler \
	-iwithprefix serial \
	-iwithprefix sortlist \
	-iwithprefix stack_info \
	-iwithprefix strerror \
	-iwithprefix twi_sensor \
	-iwithprefix twi_mngr \
	-iwithprefix timer \
	-iwithprefix util \
	-iwithprefix usbd \
	-iwithprefix usbd\class\cdc \
	-iwithprefix usbd\class\cdc\acm \
	-iprefix $(NRF52_SDK_ROOT)\ \
	-iwithprefix external\nrf_cc310\include \
	-iwithprefix external\nrf_cc310_bl\include \
	-iwithprefix external\freertos\portable\GCC\nrf52 \
	-iwithprefix external\freertos\portable\CMSIS\nrf52 \
	-iwithprefix external\freertos\source \
	-iwithprefix external\freertos\source\include \
	-iwithprefix external\freertos\source\portable\MemMang \
	-iwithprefix components \
	-iwithprefix components\ble\common \
	-iwithprefix components\ble\ble_advertising \
	-iwithprefix components\ble\ble_radio_notification \
	-iwithprefix components\ble\nrf_ble_gatt \
	-iwithprefix components\ble\nrf_ble_qwr \
	-iwithprefix components\ble\nrf_ble_scan \
	-iwithprefix components\ble\peer_manager \
	-iwithprefix components\boards \
	-iwithprefix components\softdevice\common \
	-iwithprefix components\softdevice\$(SOFT_DEVICE)\headers \
	-iwithprefix components\softdevice\$(SOFT_DEVICE)\headers\nrf52 \
	-iwithprefix components\toolchain\cmsis\include \
	-iwithprefix external\fprintf \
	-iwithprefix external\utf_converter \
	-iwithprefix integration\nrfx\legacy \
	-iwithprefix integration\nrfx \
	-iwithprefix modules\nrfx \
	-iwithprefix modules\nrfx\drivers\include \
	-iwithprefix modules\nrfx\drivers\src \
	-iwithprefix modules\nrfx\drivers\src\prs \
	-iwithprefix modules\nrfx\hal \
	-iwithprefix modules\nrfx\mdk \
	-iwithprefix modules\nrfx\soc

SDK_GLUE_INCLUDES = \
	-I$(PLATFORM_DIR) \
	-I$(PLATFORM_DIR)\config

XS_INCLUDES = \
	-iprefix $(XS_DIR)\ \
	-iwithprefix includes \
	-iwithprefix sources \
	-iwithprefix platforms\mc \
	-iwithprefix platforms\nrf52 \
	-iwithprefix ..\modules\files\preference \
	-iwithprefix ..\modules\base\instrumentation \
	-iwithprefix ..\modules\base\timer \
	-iprefix $(BUILD_DIR)\devices\nrf52\ \
	-iwithprefix base \
	-iwithprefix config \
	-iwithprefix xsProj

BOARD_SUPPORT_OBJ = \
	$(LIB_DIR)\boards.o \
	$(LIB_DIR)\bsp.o \
	$(LIB_DIR)\bsp_btn_ble.o

FREERTOS_OBJ = \
	$(LIB_DIR)\croutine.o \
	$(LIB_DIR)\event_groups.o \
	$(LIB_DIR)\heap_4.o \
	$(LIB_DIR)\list.o \
	$(LIB_DIR)\port.o \
	$(LIB_DIR)\port_cmsis.o \
	$(LIB_DIR)\port_cmsis_systick.o \
	$(LIB_DIR)\queue.o \
	$(LIB_DIR)\stream_buffer.o \
	$(LIB_DIR)\tasks.o \
	$(LIB_DIR)\timers.o

NRF_BLE_OBJ = \
	$(LIB_DIR)\auth_status_tracker.o \
	$(LIB_DIR)\ble_advdata.o \
	$(LIB_DIR)\ble_advertising.o \
	$(LIB_DIR)\ble_conn_params.o \
	$(LIB_DIR)\ble_conn_state.o \
	$(LIB_DIR)\ble_radio_notification.o \
	$(LIB_DIR)\ble_srv_common.o \
	$(LIB_DIR)\gatt_cache_manager.o \
	$(LIB_DIR)\gatts_cache_manager.o \
	$(LIB_DIR)\id_manager.o \
	$(LIB_DIR)\nrf_ble_gatt.o \
	$(LIB_DIR)\nrf_ble_lesc.o \
	$(LIB_DIR)\nrf_ble_qwr.o \
	$(LIB_DIR)\nrf_ble_scan.o \
	$(LIB_DIR)\peer_data_storage.o \
	$(LIB_DIR)\peer_database.o \
	$(LIB_DIR)\peer_id.o \
	$(LIB_DIR)\peer_manager.o \
	$(LIB_DIR)\peer_manager_handler.o \
	$(LIB_DIR)\pm_buffer.o \
	$(LIB_DIR)\security_dispatcher.o \
	$(LIB_DIR)\security_manager.o

NRF_CRYPTO_OBJ = \
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

NRF_DRIVERS_OBJ = \
	$(LIB_DIR)\nrf_drv_clock.o \
	$(LIB_DIR)\nrf_drv_power.o \
	$(LIB_DIR)\nrf_drv_twi.o \
	$(LIB_DIR)\nrfx_atomic.o \
	$(LIB_DIR)\nrfx_clock.o \
	$(LIB_DIR)\nrfx_gpiote.o \
	$(LIB_DIR)\nrfx_lpcomp.o \
	$(LIB_DIR)\nrfx_power.o \
	$(LIB_DIR)\nrfx_ppi.o \
	$(LIB_DIR)\nrfx_prs.o \
	$(LIB_DIR)\nrfx_pwm.o \
	$(LIB_DIR)\nrfx_qdec.o \
	$(LIB_DIR)\nrfx_saadc.o \
	$(LIB_DIR)\nrfx_spim.o \
	$(LIB_DIR)\nrfx_spis.o \
	$(LIB_DIR)\nrfx_systick.o \
	$(LIB_DIR)\nrfx_timer.o \
	$(LIB_DIR)\nrfx_twim.o \
	$(LIB_DIR)\nrfx_uarte.o \
	$(LIB_DIR)\nrfx_wdt.o

NRF_CRYPTO_BACKEND_CC310_OBJ = \
	$(LIB_DIR)\cc310_backend_aes.o \
	$(LIB_DIR)\cc310_backend_aes_aead.o \
	$(LIB_DIR)\cc310_backend_chacha_poly_aead.o \
	$(LIB_DIR)\cc310_backend_ecc.o \
	$(LIB_DIR)\cc310_backend_ecdh.o \
	$(LIB_DIR)\cc310_backend_ecdsa.o \
	$(LIB_DIR)\cc310_backend_hash.o \
	$(LIB_DIR)\cc310_backend_hmac.o \
	$(LIB_DIR)\cc310_backend_init.o \
	$(LIB_DIR)\cc310_backend_mutex.o \
	$(LIB_DIR)\cc310_backend_rng.o \
	$(LIB_DIR)\cc310_backend_shared.o

NRF_HW_CRYPTO_BACKEND_OBJ = \
	$(LIB_DIR)\nrf_hw_backend_init.o \
	$(LIB_DIR)\nrf_hw_backend_rng.o \
	$(LIB_DIR)\nrf_hw_backend_rng_mbedtls.o

NRF_LIBRARIES_OBJ = \
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
	$(LIB_DIR)\app_fifo.o \
	$(LIB_DIR)\nrf_fprintf.o \
	$(LIB_DIR)\nrf_fprintf_format.o \
	$(LIB_DIR)\nrf_fstorage_sd.o \
	$(LIB_DIR)\nrf_fstorage.o \
	$(LIB_DIR)\nrf_memobj.o \
	$(LIB_DIR)\nrf_queue.o \
	$(LIB_DIR)\nrf_ringbuf.o \
	$(LIB_DIR)\nrf_section_iter.o \
	$(LIB_DIR)\nrf_strerror.o \
	$(LIB_DIR)\nrf_twi_mngr.o \
	$(LIB_DIR)\nrf_twi_sensor.o

#	$(LIB_DIR)\nrf_libuarte_drv.o
#	$(LIB_DIR)\nrf_libuarte_async.o

NRF_LOG_OBJ = \
	$(LIB_DIR)\nrf_log_backend_rtt.o \
	$(LIB_DIR)\nrf_log_backend_serial.o \
	$(LIB_DIR)\nrf_log_backend_uart.o \
	$(LIB_DIR)\nrf_log_default_backends.o \
	$(LIB_DIR)\nrf_log_frontend.o \
	$(LIB_DIR)\nrf_log_str_formatter.o

NRF_SOFTDEVICE_OBJ = \
	$(LIB_DIR)\nrf_sdh.o \
	$(LIB_DIR)\nrf_sdh_ble.o \
	$(LIB_DIR)\nrf_sdh_freertos.o \
	$(LIB_DIR)\nrf_sdh_soc.o

NRF_USBD_OBJ = \
	$(LIB_DIR)\app_usbd.o \
	$(LIB_DIR)\app_usbd_cdc_acm.o \
	$(LIB_DIR)\app_usbd_core.o \
	$(LIB_DIR)\app_usbd_serial_num.o \
	$(LIB_DIR)\app_usbd_string_desc.o \
	$(LIB_DIR)\nrfx_usbd.o

SDK_GLUE_OBJ = \
	$(TMP_DIR)\debugger.o \
	$(TMP_DIR)\debugger_usbd.o \
	$(TMP_DIR)\ftdi_trace.o \
	$(TMP_DIR)\main.o \
	$(TMP_DIR)\systemclock.o \
	$(TMP_DIR)\xsmain.o \
	$(TMP_DIR)\app_usbd_vendor.o

#	$(TMP_DIR)\nrf52_serial.o 

STARTUP_OBJ = \
	$(LIB_DIR)\moddable_startup_nrf52840.o \
	$(LIB_DIR)\hardfault_handler_gcc.o \
	$(LIB_DIR)\hardfault_implementation.o \
	$(LIB_DIR)\system_nrf52840.o

#	$(LIB_DIR)\gcc_startup_nrf52840.o

XS_OBJ = \
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
	$(OBJECTS) \
	$(BOARD_SUPPORT_OBJ) \
	$(FREERTOS_OBJ) \
	$(NRF_BLE_OBJ) \
	$(NRF_CRYPTO_OBJ) \
	$(NRF_CRYPTO_BACKEND_CC310_OBJ) \
	$(NRF_DRIVERS_OBJ) \
	$(NRF_HW_CRYPTO_BACKEND_OBJ) \
	$(NRF_LOG_OBJ) \
	$(NRF_LIBRARIES_OBJ) \
	$(NRF_SOFTDEVICE_OBJ) \
	$(NRF_USBD_OBJ) \
	$(STARTUP_OBJ)

FINAL_LINK_OBJ = \
	$(LIB_DIR)\buildinfo.o \
	$(TMP_DIR)\xsHost.o \
	$(TMP_DIR)\xsHosts.o \
	$(TMP_DIR)\xsPlatform.o \
	$(TMP_DIR)\xsDebug.o \
	$(OBJECTS) \
	$(SDK_GLUE_OBJ) \
	$(TMP_DIR)\mc.xs.o \
	$(TMP_DIR)\mc.resources.o \
	$(XS_OBJ)

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
	-lc \
	-lnosys \
	-lm \
	$(NRF52_SDK_ROOT)\external\nrf_cc310\lib\cortex-m4\hard-float\no-interrupts\libnrf_cc310_0.9.13.a

NRF_C_DEFINES = \
	-D__SIZEOF_WCHAR_T=4 \
	-D__ARM_ARCH_7EM__ \
	-D__ARM_ARCH_FPV4_SP_D16__ \
	-D__HEAP_SIZE__=$(NRF52_HEAP_SIZE) \
	-D__GNU_LINKER \
	-D$(BOARD_DEF) \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DFREERTOS \
	-DINCLUDE_vTaskSuspend \
	-DINITIALIZE_USER_SECTIONS \
	-DNO_VTOR_CONFIG  \
	-DNRF52840_XXAA \
	-DNRF_SD_BLE_API_VERSION=7 \
	-DNRF_USBD_REQUIRE_CLOSED_ON_PORT_OPEN=1 \
	-DS140 \
	-DSOFTDEVICE_PRESENT \
	-Dnrf52 \
	-DAPP_TIMER_V2 \
	-DAPP_TIMER_V2_RTC1_ENABLED \
	-DMBEDTLS_CONFIG_FILE=\"nrf_crypto_mbedtls_config.h\" \
	-DNRF_CRYPTO_MAX_INSTANCE_COUNT=1 \
	-DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION

C_DEFINES = \
	$(NRF_C_DEFINES) \
	$(NET_CONFIG_FLAGS) \
	-DmxUseDefaultSharedChunks=1 \
	-DmxRun=1 \
	-DkCommodettoBitmapFormat=$(COMMODETTOBITMAPFORMAT) \
	-DkPocoRotation=$(POCOROTATION) \
	-DMODGCC=1 \
	$(FTDI_TRACE)
!IF "$(INSTRUMENT)"=="1"
C_DEFINES = $(C_DEFINES) -DMODINSTRUMENTATION=1 -DmxInstrument=1
!ENDIF
!IF "$(DEBUG)"=="1"
C_DEFINES = $(C_DEFINES) -DmxDebug=1 -DDEBUG=1 $(DEBUGGER_USBD) -g2 -Os
!ELSE
C_DEFINES = $(C_DEFINES) -Os -DUSE_WATCHDOG=0
!ENDIF

HW_DEBUG_OPT = $(FP_OPTS)
HW_OPT = -O2 $(FP_OPTS)

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
!IF "$(DEBUG)"=="1"
C_FLAGS = $(C_FLAGS) $(HW_DEBUG_OPT)
!ELSE
C_FLAGS = $(C_FLAGS) $(HW_OPT)
!ENDIF

C_FLAGS_NODATASECTION = $(C_FLAGS)

ASMFLAGS = \
	$(ASMFLAGS) \
	-mcpu=cortex-m4 \
	-mthumb \
	-mabi=aapcs \
	-D$(BOARD_DEF) \
	-DAPP_TIMER_V2 \
	-DAPP_TIMER_V2_RTC1_ENABLED \
	-DCONFIG_GPIO_AS_PINRESET \
	-DFLOAT_ABI_HARD \
	-DNRF52840_XXAA \
	$(FP_OPTS) \
	-DFREERTOS \
	-DNRF_SD_BLE_API_VERSION=7 \
	-DS140 \
	-DSOFTDEVICE_PRESENT \
	-DNRF_CRYPTO_MAX_INSTANCE_COUNT=1 \
	-DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION \
	-D__HEAP_SIZE=$(NRF52_HEAP_SIZE) \
	-D__STACK_SIZE=512

LDFLAGS = \
	-mabi=aapcs \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-mthumb \
	--specs=nano.specs \
	-L$(NRF52_SDK_ROOT)\modules\nrfx\mdk \
	-T$(LINKER_SCRIPT) \
	-Wl,--gc-sections \
	-Wl,--no-warn-rwx-segments \
	-Xlinker -no-enum-size-warning \
	-Xlinker -Map=$(BIN_DIR)\xs_lib.map

C_INCLUDES = $(GCC_INCLUDES) $(C_INCLUDES) $(DIRECTORIES) $(SDK_GLUE_INCLUDES) $(XS_INCLUDES) -I$(LIB_DIR) -I$(TMP_DIR) -I$(PLATFORM_DIR) $(NRF_SDK_INCLUDES)


.PHONY: all
.SUFFIXES: .S .s

precursor: $(BLE) $(TMP_DIR) $(LIB_DIR) $(BIN_DIR)\xs_nrf52.hex

all: precursor $(BIN_DIR)\xs_nrf52.uf2
	$(KILL_SERIAL_2_XSBUG)
	$(WAIT_FOR_M4)
	$(DO_XSBUG)
	@echo Copying: $(BIN_DIR)\xs_nrf52.hex to $(UF2_VOLUME_NAME)
	$(DO_COPY)
	$(WAIT_FOR_NEW_SERIAL)
	$(SERIAL_2_XSBUG) $(NORESTART)

clean:
	echo # Clean project
	echo $(BIN_DIR)
	if exist $(BIN_DIR) del /s/q/f $(BIN_DIR)\*.* > NUL
	if exist $(BIN_DIR) rmdir /s/q $(BIN_DIR)
	echo $(TMP_DIR)
	if exist $(TMP_DIR) del /s/q/f $(TMP_DIR)\*.* > NUL
	if exist $(TMP_DIR) rmdir /s/q $(TMP_DIR)
	echo $(LIB_DIR)
	if exist $(LIB_DIR) del /s/q/f $(LIB_DIR)\*.* > NUL
	if exist $(LIB_DIR) rmdir /s/q $(LIB_DIR)

NRFJPROG_ARGS = -f nrf52 --qspiini $(QSPI_INI_PATH_WIN) --log
flash: precursor $(BIN_DIR)\xs_nrf52.hex
	@echo Flashing: $(BIN_DIR)\xs_nrf52.hex
	@echo # $(NRFJPROG) $(NRFJPROG_ARGS) --program $(BIN_DIR)\xs_nrf52.hex --qspisectorerase --sectorerase
	$(NRFJPROG) $(NRFJPROG_ARGS) --program $(BIN_DIR)\xs_nrf52.hex --qspisectorerase --sectorerase
	@echo # $(NRFJPROG) $(NRFJPROG_ARGS) --verify $(BIN_DIR)\xs_nrf52.hex
	$(NRFJPROG) $(NRFJPROG_ARGS) --verify $(BIN_DIR)\xs_nrf52.hex
	@echo # $(NRFJPROG) --reset
	$(NRFJPROG) --reset

debugger:
	$(DO_XSBUG)
	$(MODDABLE_TOOLS_DIR)\serial2xsbug $(DEBUGGER_PORT) $(DEBUGGER_SPEED) 8N1 -dtr $(NORESTART)

build: precursor $(BIN_DIR)\xs_nrf52.uf2
	@echo Target built: $(BIN_DIR)\xs_nrf52.uf2

brin: flash debugger

use_jlink: flash debugger

xsbug:
	$(KILL_SERIAL_2_XSBUG)
	$(DO_XSBUG)
	$(WAIT_FOR_NEW_SERIAL)
	$(SERIAL_2_XSBUG)

$(BIN_DIR)\xs_nrf52.uf2: $(BIN_DIR)\xs_nrf52.hex
	@echo Making: $(BIN_DIR)\xs_nrf52.uf2 from xs_nrf52.hex
	python $(UF2CONV) $(BIN_DIR)\xs_nrf52.hex -c -f 0xADA52840 -o $(BIN_DIR)\xs_nrf52.uf2

$(TMP_DIR):
	if not exist $(TMP_DIR)\$(NULL) mkdir $(TMP_DIR)

$(LIB_DIR):
	if not exist $(LIB_DIR)\$(NULL) mkdir $(LIB_DIR)

$(LIB_DIR)\buildinfo.h:
	echo typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo; > $(LIB_DIR)\buildinfo.h
	
$(BIN_DIR)\xs_nrf52.bin: $(TMP_DIR)\xs_nrf52.hex
	$(OBJCOPY) -O binary $(TMP_DIR)\xs_nrf52.out $(BIN_DIR)\xs_nrf52.bin

$(BIN_DIR)\xs_nrf52.hex: $(TMP_DIR)\xs_nrf52.out
	$(SIZE) -A $(TMP_DIR)\xs_nrf52.out > $(TMP_DIR)\_size.tmp
	$(ECHO_GIT_AND_SIZE) $(USE_QSPI)
	$(OBJCOPY) -O ihex $(TMP_DIR)\xs_nrf52.out $(BIN_DIR)\xs_nrf52.hex

$(TMP_DIR)\xs_nrf52.out: $(FINAL_LINK_OBJ)
	@echo creating xs_nrf52.out
	if exist $(TMP_DIR)\xs_nrf52.out del /s/q/f $(TMP_DIR)\xs_nrf52.out > NUL
	@echo link to .out file
	$(LD) $(LDFLAGS) $(FINAL_LINK_OBJ) $(LIB_FILES) -o $@

$(LIB_DIR)\buildinfo.o: $(SDK_GLUE_OBJ) $(XS_OBJ) $(TMP_DIR)\mc.xs.o $(TMP_DIR)\mc.resources.o $(OBJECTS) $(LIB_DIR)\buildinfo.h
	@echo # buildinfo
	echo #include "buildinfo.h" > $(LIB_DIR)\buildinfo.c
	echo _tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"}; >> $(LIB_DIR)\buildinfo.c
	$(CC) $(C_FLAGS) $(C_INCLUDES) $(C_DEFINES) $(LIB_DIR)\buildinfo.c -o $@

$(LIB_DIR)\moddable_startup_nrf52840.o: $(BUILD_DIR)\devices\nrf52\xsProj\moddable_startup_nrf52840.S
	@echo # asm $(@F)
	$(CC) -c -x assembler-with-cpp $(ASMFLAGS) $(C_INCLUDES) $? -o $@

{$(TMP_DIR)\}.c{$(TMP_DIR)\}.o:
	@echo # application: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $? -o $@

$(XS_OBJ): $(XS_HEADERS)
{$(XS_DIR)\sources\}.c{$(LIB_DIR)\}.o:
	@echo # library xs: $(@F)
	$(CC) -c $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

$(TMP_DIR)\xsDebug.o: $(XS_DIR)\sources\xsDebug.c $(TMP_DIR)\mc.defines.h
	@echo # project xs: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $(XS_DIR)\sources\xsDebug.c -o $@

$(TMP_DIR)\xsHosts.o: $(XS_DIR)\platforms\mc\xsHosts.c $(TMP_DIR)\mc.defines.h
	@echo # project xs: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $(XS_DIR)\platforms\mc\xsHosts.c -o $@

$(TMP_DIR)\xsHost.o: $(XS_DIR)\platforms\nrf52\xsHost.c $(TMP_DIR)\mc.defines.h
	@echo # project xs: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $(XS_DIR)\platforms\nrf52\xsHost.c -o $@

$(TMP_DIR)\xsPlatform.o: $(XS_DIR)\platforms\nrf52\xsPlatform.c $(TMP_DIR)\mc.defines.h
	@echo # project xs: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $(XS_DIR)\platforms\nrf52\xsPlatform.c -o $@

{$(NRF52_SDK_ROOT)\components\ble\ble_advertising\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\ble_radio_notification\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\common\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\nrf_ble_gatt\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\nrf_ble_lesc\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\nrf_ble_qwr\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\nrf_ble_scan\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\ble\peer_manager\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\boards\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\atomic\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\atomic_fifo\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\atomic_flags\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\balloc\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\bsp\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\button\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\crypto\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\crypto\backend\cc310\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\crypto\backend\nrf_hw\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\experimental_section_vars\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\fds\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\fifo\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\fstorage\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\hardfault\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\hardfault\nrf52\handler\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\libuarte\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\log\src\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\memobj\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\queue\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\ringbuf\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\serial\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\spi_mngr\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\strerror\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\timer\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\twi_mngr\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\twi_sensor\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\usbd\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\usbd\class\cdc\acm\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\libraries\util\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\components\softdevice\common\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\external\fprintf\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\external\freertos\portable\CMSIS\nrf52\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\external\freertos\portable\GCC\nrf52\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\external\freertos\source\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\external\freertos\source\portable\MemMang\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\integration\nrfx\legacy\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\modules\nrfx\mdk\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\modules\nrfx\drivers\src\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\modules\nrfx\drivers\src\prs\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(NRF52_SDK_ROOT)\modules\nrfx\soc\}.c{$(LIB_DIR)\}.o:
	@echo # library: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

{$(BUILD_DIR)\devices\nrf52\base\}.c{$(TMP_DIR)\}.o:
	@echo # application: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $< -o $@

$(TMP_DIR)\main.o: $(BUILD_DIR)\devices\nrf52\xsProj\main.c
	@echo # application: $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $? -o $@

$(TMP_DIR)\mc.xs.o: $(TMP_DIR)\mc.xs.c
	@echo # cc $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $? -o $@

$(TMP_DIR)\mc.resources.o: $(TMP_DIR)\mc.resources.c
	@echo # cc $(@F)
	$(CC) $(C_FLAGS) $(C_DEFINES) $(C_INCLUDES) $? -o $@

$(TMP_DIR)\mc.xs.c: $(MODULES) $(MANIFEST)
	@echo # xsl modules
	$(XSL) -b $(MODULES_DIR) -o $(TMP_DIR) $(PRELOADS) $(STRIPS) $(CREATION) -u / $(MODULES)

$(TMP_DIR)\mc.resources.c: $(DATA) $(RESOURCES) $(MANIFEST)
	@echo # mcrez resources
	$(MCREZ) $(DATA) $(RESOURCES) -o $(TMP_DIR) -p nrf52 -r mc.resources.c
