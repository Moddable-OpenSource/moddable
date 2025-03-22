cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(test_allocator C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(test_allocator test_allocator.c)

add_subdirectory(sparkfun_pico)

pico_enable_stdio_usb(test_allocator 1)
pico_enable_stdio_uart(test_allocator 1)

# pull in common dependencies and additional spi hardware support
target_link_libraries(test_allocator pico_stdlib sparkfun_pico)

# create map/bin/hex file etc.
pico_add_extra_outputs(test_allocator)
