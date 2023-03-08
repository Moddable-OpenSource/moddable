#
# Copyright (c) 2020 Moddable Tech, Inc.
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

cmake_minimum_required(VERSION 3.5)
if (WIN32)
	set(CMAKE_OBJECT_PATH_MAX 260)
endif ()

file(TO_CMAKE_PATH "$ENV{MODDABLE}" ENV_MODDABLE)

if(MATTER_BUILD EQUAL "1")
	if(NOT DEFINED ENV{ESP_MATTER_PATH})
		message(FATAL_ERROR "Please set ESP_MATTER_PATH to the path of esp-matter repo")
	endif(NOT DEFINED ENV{ESP_MATTER_PATH})

	set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
	set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)
	set(ZAP_GENERATED_PATH ${ENV_MODDABLE}/modules/network/matter/esp32/inc/zap-generated)

	set(ENV{ESP_MATTER_DEVICE_PATH} $ENV{ESP_MATTER_PATH}/device_hal/device/esp32_devkit_c)
	set(COMPONENT_ADD_INCLUDEDIRS ${ENV_MODDABLE}/xs/platforms/esp ${ENV_MODDABLE}/xs/platforms/mc ${ENV_MODDABLE}/xs/includes ${ENV_MODDABLE}/modules/base/instrumentation ${ENV_MODDABLE}/modules/network/matter/esp32/inc ${C_TMP_DIR})
	set(COMPONENT_PRIV_INCLUDEDIRS ${ENV_MODDABLE}/modules/network/matter/esp32/inc)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if(MATTER_BUILD EQUAL "1")
	include($ENV{ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)
	set(EXTRA_COMPONENT_DIRS
		"${ESP_MATTER_PATH}/examples/common"
		"${MATTER_SDK_PATH}/config/esp32/components"
		"${ESP_MATTER_PATH}/components"
		"${ESP_MATTER_PATH}/device_hal/device"
		${extra_components_dirs_append})
endif()

project(xs_esp32)

if(MATTER_BUILD EQUAL "1")
	idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++14;-Os;-DCHIP_HAVE_CONFIG_H" APPEND)
	idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)

	# For RISCV chips, project_include.cmake sets -Wno-format, but does not clear various
	# flags that depend on -Wformat
	idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security;-Wformat=0" APPEND)
endif()