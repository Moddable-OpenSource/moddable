cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(set_q_flash C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(set_qflash set_qflash.c)

pico_enable_stdio_usb(set_qflash 1)
pico_enable_stdio_uart(set_qflash 1)

# pull in common dependencies and additional spi hardware support
target_link_libraries(set_qflash pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(set_qflash)
