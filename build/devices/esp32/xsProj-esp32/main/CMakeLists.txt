#
# Copyright (c) 2022-2023 Moddable Tech, Inc.
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


file(TO_CMAKE_PATH "$ENV{MODDABLE}" ENV_MODDABLE)
file(TO_CMAKE_PATH "${TMP_DIR}" C_TMP_DIR)
set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS ${ENV_MODDABLE}/xs/platforms/esp  ${ENV_MODDABLE}/xs/platforms/mc ${ENV_MODDABLE}/xs/includes ${ENV_MODDABLE}/modules/base/instrumentation ${C_TMP_DIR})

set(REQUIRES_LIST driver nvs_flash esp_adc_cal vfs spiffs fatfs bt esp_eth)

if (MATTER_BUILD EQUAL "1")
    set(COMPONENT_ADD_INCLUDEDIRS ${COMPONENT_ADD_INCLUDEDIRS} ${ENV_MODDABLE}/modules/network/matter/esp32/inc)
    set(COMPONENT_PRIV_INCLUDEDIRS ${ENV_MODDABLE}/modules/network/matter/esp32/inc)
    set(REQUIRES_LIST ${REQUIRES_LIST} device chip esp_matter route_hook app_reset)
endif()

register_component()

if (mxDebug EQUAL "1")
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DmxDebug=1)
endif()
if (DEBUGGER_SPEED)
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DDEBUGGER_SPEED=${DEBUGGER_SPEED})
endif()
if (INSTRUMENT EQUAL "1")
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DMODINSTRUMENTATION=1)
endif()

add_prebuilt_library(xsesp32 ${CMAKE_BINARY_DIR}/xs_${ESP32_SUBCLASS}.a
             PRIV_REQUIRES ${REQUIRES_LIST})

if (MATTER_BUILD EQUAL "1")
    set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 14)
    target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
endif()

target_link_libraries(${COMPONENT_LIB} PRIVATE xsesp32)

