# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_OBJECT_PATH_MAX 1024)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(moddable)

# export all the flags used by zephyr
zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

if(DEFINED CMAKE_C_COMPILER_TARGET)
  set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
endif()

set(external_project_cflags
  "${target_flag} ${includes} ${definitions} ${options} ${system_includes}"
  )

add_subdirectory("${MODDABLE_BUILD_DIR}" "${MODDABLE_BUILD_DIR}/app")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# message(STATUS "*-*-*-*-*-*-*-*-*")
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#      message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()


target_sources(app PRIVATE
	src/main.c
	src/debugger.c
	src/serial_fifo.c
	)

