# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
cmake_policy(SET CMP0156 NEW)
cmake_policy(SET CMP0181 NEW)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(moddable_lib)

message(" -- in moddable cmakelists.txt")
# export all the flags used by zephyr
zephyr_get_include_directories_for_lang_as_string(       C includes)
zephyr_get_system_include_directories_for_lang_as_string(C system_includes)
zephyr_get_compile_definitions_for_lang_as_string(       C definitions)
zephyr_get_compile_options_for_lang_as_string(           C options)

if(DEFINED CMAKE_C_COMPILER_TARGET)
  set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
endif()


message(" *- source " ${CMAKE_CURRENT_SOURCE_DIR} )

# add_library(moddable_lib STATIC IMPORTED GLOBAL
# add_library(moddable_lib OBJECT
# target_sources(moddable_lib

list(APPEND mOBJECTS xs/platforms/mc/xsHosts.c)
list(APPEND mOBJECTS xs/platforms/zephyr/xsHost.c)
list(APPEND mOBJECTS xs/platforms/zephyr/xsPlatform.c)
list(APPEND mOBJECTS xs/sources/xsAPI.c)
list(APPEND mOBJECTS xs/sources/xsAll.c)
list(APPEND mOBJECTS xs/sources/xsArguments.c)
list(APPEND mOBJECTS xs/sources/xsArray.c)
list(APPEND mOBJECTS xs/sources/xsAtomics.c)
list(APPEND mOBJECTS xs/sources/xsBigInt.c)
list(APPEND mOBJECTS xs/sources/xsBoolean.c)
list(APPEND mOBJECTS xs/sources/xsCode.c)
list(APPEND mOBJECTS xs/sources/xsCommon.c)
list(APPEND mOBJECTS xs/sources/xsDataView.c)
list(APPEND mOBJECTS xs/sources/xsDate.c)
list(APPEND mOBJECTS xs/sources/xsDebug.c)
list(APPEND mOBJECTS xs/sources/xsError.c)
list(APPEND mOBJECTS xs/sources/xsFunction.c)
list(APPEND mOBJECTS xs/sources/xsGenerator.c)
list(APPEND mOBJECTS xs/sources/xsGlobal.c)
list(APPEND mOBJECTS xs/sources/xsJSON.c)
list(APPEND mOBJECTS xs/sources/xsLexical.c)
list(APPEND mOBJECTS xs/sources/xsLockdown.c)
list(APPEND mOBJECTS xs/sources/xsMapSet.c)
list(APPEND mOBJECTS xs/sources/xsMarshall.c)
list(APPEND mOBJECTS xs/sources/xsMath.c)
list(APPEND mOBJECTS xs/sources/xsMemory.c)
list(APPEND mOBJECTS xs/sources/xsModule.c)
list(APPEND mOBJECTS xs/sources/xsNumber.c)
list(APPEND mOBJECTS xs/sources/xsObject.c)
list(APPEND mOBJECTS xs/sources/xsPlatforms.c)
list(APPEND mOBJECTS xs/sources/xsProfile.c)
list(APPEND mOBJECTS xs/sources/xsPromise.c)
list(APPEND mOBJECTS xs/sources/xsProperty.c)
list(APPEND mOBJECTS xs/sources/xsProxy.c)
list(APPEND mOBJECTS xs/sources/xsRegExp.c)
list(APPEND mOBJECTS xs/sources/xsRun.c)
list(APPEND mOBJECTS xs/sources/xsScope.c)
list(APPEND mOBJECTS xs/sources/xsScript.c)
list(APPEND mOBJECTS xs/sources/xsSnapshot.c)
list(APPEND mOBJECTS xs/sources/xsSourceMap.c)
list(APPEND mOBJECTS xs/sources/xsString.c)
list(APPEND mOBJECTS xs/sources/xsSymbol.c)
list(APPEND mOBJECTS xs/sources/xsSyntaxical.c)
list(APPEND mOBJECTS xs/sources/xsTree.c)
list(APPEND mOBJECTS xs/sources/xsType.c)
list(APPEND mOBJECTS xs/sources/xsdtoa.c)
list(APPEND mOBJECTS xs/sources/xsmc.c)
list(APPEND mOBJECTS xs/sources/xsre.c)
# list(APPEND mOBJECTS modules/io/common/builtinCommon.c)
list(APPEND mOBJECTS modules/files/resource/Resource.c)
list(APPEND mOBJECTS modules/base/time/zephyr/modTime.c)
list(APPEND mOBJECTS modules/base/timer/modTimer.c)
# list(APPEND mOBJECTS modules/base/timer/zephyr/timer.c)
list(APPEND mOBJECTS modules/base/timer/mc/timer.c)
list(APPEND mOBJECTS build/tmp/zephyr/release/helloworld/mc.xs.c)
list(APPEND mOBJECTS build/tmp/zephyr/release/helloworld/mc.resources.c)

list(APPEND mFLAGS "-D_ZEPHYR=1")
list(APPEND mFLAGS "-DmxUseDefaultSharedChunks=1")
list(APPEND mFLAGS "-DkCommodettoBitmapFormat=kCommodettoBitmapMonochromeAligned")
list(APPEND mFLAGS "-DkPocoRotation=0")
list(APPEND mFLAGS "-DkPocoFrameBuffer=1")
list(APPEND mFLAGS "-DMODGCC=1")
list(APPEND mFLAGS "-DMODINSTRUMENTATION=0")
list(APPEND mFLAGS "-DmxUseDefaultSharedChunks=1")
# list(APPEND mFLAGS "-fno-strict-aliasing")
# list(APPEND mFLAGS "-fomit-frame-pointer")
list(APPEND mOPTIONS "-fshort-enums")
# list(APPEND mFLAGS "-mlittle-endian")
# list(APPEND mFLAGS "-mthumb-interwork")
# list(APPEND mFLAGS "-mtp=soft")
# list(APPEND mOPTIONS "-munaligned-access")
list(APPEND mOPTIONS "-Wno-error=clobbered")
list(APPEND mOPTIONS "-Wno-error=sign-compare")
list(APPEND mOPTIONS "-Wno-error=strict-aliasing")
list(APPEND mOPTIONS "-Wno-misleading-indentation")

# add_library(moddable_lib STATIC ${mOBJECTS} ${SOURCES})
zephyr_library_sources(${mOBJECTS})
zephyr_library_compile_definitions(${mFLAGS})
zephyr_library_compile_options(${mOPTIONS})

# target_include_directories(moddable_lib
zephyr_library_include_directories(
        xs/includes
        xs/sources
        xs/platforms/mc
        xs/platforms/zephyr
        xs/tools/fdlibm
        modules/base/timer
        modules/base/instrumentation
        modules/commodetto/gif
        modules/commodetto
        modules/io/common
        modules/piu/All
        modules/piu/MC
        build/tmp/zephyr/release/helloworld
	)

# message(" -- includes ${includes}" )
# message(" -- system_includes ${system_includes}" )
# message(" -- target_flag ${target_flag} ")
# message(" -- definitions ${definitions} ")
# message(" -- options ${options} ")

# add_custom_command(
# 	OUTPUT build/bin/zephyr/release/lib/libmoddable_lib.a
# )
# set(CMAKE_C_FLAGS ${definitions} ${options} ${includes} ${system_includes})

# message(STATUS "*-*-*-*-*-*-*-*-*")
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

