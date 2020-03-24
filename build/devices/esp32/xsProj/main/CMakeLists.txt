set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS $ENV{MODDABLE}/xs/platforms/esp $ENV{MODDABLE}/xs/includes $ENV{MODDABLE}/modules/base/instrumentation)

register_component()

if (mxDebug EQUAL "1")
    target_compile_options(${COMPONENT_TARGET} PRIVATE -DmxDebug=1)
endif()
target_link_libraries(${COMPONENT_TARGET} ${CMAKE_BINARY_DIR}/xs_esp32.a)