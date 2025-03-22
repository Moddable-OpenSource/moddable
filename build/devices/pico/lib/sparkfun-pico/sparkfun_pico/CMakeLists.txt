add_library(sparkfun_pico INTERFACE)

target_sources(
    sparkfun_pico
    INTERFACE ${CMAKE_CURRENT_LIST_DIR}/sfe_pico_alloc.c
              ${CMAKE_CURRENT_LIST_DIR}/sfe_pico_alloc.h
              ${CMAKE_CURRENT_LIST_DIR}/sfe_pico.h
              ${CMAKE_CURRENT_LIST_DIR}/sfe_pico_boards.h
              ${CMAKE_CURRENT_LIST_DIR}/sfe_psram.h
              ${CMAKE_CURRENT_LIST_DIR}/sfe_psram.c
              ${CMAKE_CURRENT_LIST_DIR}/tlsf/tlsf.c
              ${CMAKE_CURRENT_LIST_DIR}/tlsf/tlsf.h
              ${CMAKE_CURRENT_LIST_DIR}/tlsf/tlsf_common.h
              ${CMAKE_CURRENT_LIST_DIR}/tlsf/tlsf_block_functions.h)

# Are we wrapping the built in alloc functions?
if (SFE_PICO_ALLOC_WRAP)
    pico_wrap_function(sparkfun_pico malloc)
    pico_wrap_function(sparkfun_pico calloc)
    pico_wrap_function(sparkfun_pico realloc)
    pico_wrap_function(sparkfun_pico free)
endif ()

target_link_libraries(sparkfun_pico INTERFACE pico_stdlib hardware_spi hardware_gpio pico_flash
                                              hardware_exception hardware_sync)
