#
# Copyright (c) 2020-2022 Moddable Tech, Inc.
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
set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS ${ENV_MODDABLE}/xs/platforms/esp  ${ENV_MODDABLE}/xs/platforms/mc ${ENV_MODDABLE}/xs/includes ${ENV_MODDABLE}/modules/base/instrumentation)

register_component()

if (mxDebug EQUAL "1")
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DmxDebug=1)
endif()
if (DEBUGGER_SPEED)
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DDEBUGGER_SPEED=${DEBUGGER_SPEED})
endif()

add_prebuilt_library(xs_esp32 ${CMAKE_BINARY_DIR}/xs_${ESP32_SUBCLASS}.a
             PRIV_REQUIRES driver nvs_flash vfs spiffs fatfs bt)

target_link_libraries(${COMPONENT_LIB} PRIVATE xs_esp32)
