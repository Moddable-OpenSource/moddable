cmake_minimum_required(VERSION 3.13)

# use our own allocator

# the following enables the system malloc/free to be wrapped
set(SKIP_PICO_MALLOC 1)

# the following enables wrapping in sparkfun_pico builds
set(SFE_PICO_ALLOC_WRAP 1)

# the following enables the system malloc/free to be wrapped during compilation
add_definitions(-DSFE_PICO_ALLOC_WRAP)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(all_allocator C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(all_allocator all_allocator.c test_cpp.cpp)

add_subdirectory(sparkfun_pico)

pico_enable_stdio_usb(all_allocator 1)
pico_enable_stdio_uart(all_allocator 1)

# pull in common dependencies and additional spi hardware support
target_link_libraries(all_allocator pico_stdlib sparkfun_pico)

# create map/bin/hex file etc.
pico_add_extra_outputs(all_allocator)
