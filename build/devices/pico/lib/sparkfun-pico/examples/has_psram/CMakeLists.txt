cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(has_psram C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(has_psram has_psram.c)

# To run example, copy over the sparkfun_pico folder
add_subdirectory(sparkfun_pico)

pico_enable_stdio_usb(has_psram 1)
pico_enable_stdio_uart(has_psram 1)

# pull in common dependencies and additional spi hardware support
target_link_libraries(has_psram pico_stdlib sparkfun_pico)

# create map/bin/hex file etc.
pico_add_extra_outputs(has_psram)
