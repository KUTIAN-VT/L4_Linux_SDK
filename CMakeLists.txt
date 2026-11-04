cmake_minimum_required(VERSION 3.22)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" L4_SDK_VERSION_FULL LIMIT_COUNT 1)
string(STRIP "${L4_SDK_VERSION_FULL}" L4_SDK_VERSION_FULL)
if(NOT L4_SDK_VERSION_FULL MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)([-+][0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "Invalid SDK version in VERSION: ${L4_SDK_VERSION_FULL}")
endif()
set(L4_SDK_VERSION_CORE "${CMAKE_MATCH_1}")

project(L4_Linux_SDK VERSION ${L4_SDK_VERSION_CORE} LANGUAGES C CXX)

# Options that are still meaningful in the trimmed Linux SDK.
option(ENABLE_UDS "Enable Unix domain socket support" OFF)
option(USING_8030USB "Enable 8030 USB backend" ON)
option(USING_8030SDIO "Enable 8030 SDIO backend" OFF)
option(USING_8030UART "Enable 8030 UART backend" ON)
option(USING_XDS_HDR "Process packets with XDS header" ON)
option(APP_STATIC_LIB "Build ar8030_client as a static library instead of shared" OFF)

if(WIN32 OR ANDROID)
    message(FATAL_ERROR "This trimmed SDK is intended for Linux builds only.")
endif()

set(CMAKE_VERBOSE_MAKEFILE OFF)

message(STATUS "Configuring L4 Linux SDK ${L4_SDK_VERSION_FULL}")

if(USING_8030USB)
    add_compile_definitions(DEV_8030_USB)
    message(STATUS "daemon backend: 8030 USB")
endif()

if(USING_8030SDIO)
    add_compile_definitions(DEV_8030_SDIO)
    message(STATUS "daemon backend: 8030 SDIO")
endif()

if(USING_8030UART)
    add_compile_definitions(DEV_8030_UART)
    message(STATUS "daemon backend: 8030 UART")
endif()

if(NOT USING_8030USB AND NOT USING_8030SDIO AND NOT USING_8030UART)
    message(FATAL_ERROR "At least one 8030 backend must be enabled.")
endif()

if(USING_XDS_HDR)
    add_compile_definitions(PKT_XDS_HDR)
    message(STATUS "packet format: XDS header enabled")
endif()

if(ENABLE_UDS)
    add_compile_definitions(ENABLE_UDS)
endif()

add_subdirectory(com)
include_directories(com)
link_directories(com)

add_subdirectory(app)
add_subdirectory(examples)
add_subdirectory(daemon)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
        DESTINATION share/L4_Linux_SDK)
