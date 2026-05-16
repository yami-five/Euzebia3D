# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(EUZEBIA3D_PLATFORM "PICO" CACHE STRING "Target platform. Supported values: PICO, WINDOWS")
set(EUZEBIA3D_SUPPORTED_PLATFORMS PICO WINDOWS)
set_property(CACHE EUZEBIA3D_PLATFORM PROPERTY STRINGS ${EUZEBIA3D_SUPPORTED_PLATFORMS})
string(TOUPPER "${EUZEBIA3D_PLATFORM}" EUZEBIA3D_PLATFORM_NORMALIZED)

list(FIND EUZEBIA3D_SUPPORTED_PLATFORMS "${EUZEBIA3D_PLATFORM_NORMALIZED}" EUZEBIA3D_PLATFORM_INDEX)
if(EUZEBIA3D_PLATFORM_INDEX EQUAL -1)
    message(FATAL_ERROR "Unsupported EUZEBIA3D_PLATFORM='${EUZEBIA3D_PLATFORM}'. Supported values: ${EUZEBIA3D_SUPPORTED_PLATFORMS}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EUZEBIA3D_BUILD_APP "Build the Euzebia3D demo executable" ON)

if(EUZEBIA3D_PLATFORM_NORMALIZED STREQUAL "PICO")
    # Initialise pico_sdk from installed location
    # (note this can come from environment, CMake cache etc)

    # == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
    if(WIN32)
        set(USERHOME $ENV{USERPROFILE})
    else()
        set(USERHOME $ENV{HOME})
    endif()
    set(sdkVersion 2.2.0)
    set(toolchainVersion 14_2_Rel1)
    set(picotoolVersion 2.2.0)
    set(PICO_EXTRAS_FETCH_FROM_GIT ON)
    set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
    if(EXISTS ${picoVscode})
        include(${picoVscode})
    endif()
    # ====================================================================================
    set(PICO_BOARD pico2 CACHE STRING "Board type")

    # Pull in Raspberry Pi Pico SDK (must be before project)
    include(pico_sdk_import.cmake)
    include(pico_extras_import.cmake)

    project(Euzebia3D C CXX ASM)

    # Initialise the Raspberry Pi Pico SDK
    pico_sdk_init()

    # Platform-selected libraries
    set(EUZEBIA3D_LIBS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/libs")
    add_subdirectory("${EUZEBIA3D_LIBS_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pico_libs")

    if(EUZEBIA3D_BUILD_APP)
        # Add executable. Default name is the project name, version 0.1
        add_executable(Euzebia3D Euzebia3D.c)

        pico_set_program_name(Euzebia3D "Euzebia3D")
        pico_set_program_version(Euzebia3D "0.1")

        # Generate PIO header
        pico_generate_pio_header(Euzebia3D ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

        # Modify the below lines to enable/disable output over UART/USB
        pico_enable_stdio_uart(Euzebia3D 1)
        pico_enable_stdio_usb(Euzebia3D 1)

        # Add the standard library to the build
        target_link_libraries(Euzebia3D
                pico_stdlib)

        # Add the standard include files to the build
        target_include_directories(Euzebia3D PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}
                ${CMAKE_CURRENT_LIST_DIR}/storage
        )

        # Add any user requested libraries
        target_link_libraries(Euzebia3D
                hardware_spi
                hardware_i2c
                hardware_dma
                hardware_pio
                hardware_interp
                Euzebia3D_libs
                )

        pico_add_extra_outputs(Euzebia3D)
    endif()

elseif(EUZEBIA3D_PLATFORM_NORMALIZED STREQUAL "WINDOWS")
    project(Euzebia3D C)

    set(EUZEBIA3D_WINDOWS_MAIN "${CMAKE_CURRENT_LIST_DIR}/main_windows.c")
    if(NOT EXISTS "${EUZEBIA3D_WINDOWS_MAIN}")
        message(FATAL_ERROR "Missing Windows main file in ${EUZEBIA3D_WINDOWS_MAIN}")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/libs/CMakeLists.txt")
        message(FATAL_ERROR "Missing libraries in ${CMAKE_CURRENT_LIST_DIR}/libs")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/storage/storage.c")
        message(FATAL_ERROR "Missing storage implementation in ${CMAKE_CURRENT_LIST_DIR}/storage/storage.c")
    endif()

    find_package(SDL3 CONFIG REQUIRED)

    # Libraries are always built from Euzebia3D/libs.
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs" "${CMAKE_CURRENT_BINARY_DIR}/windows_libs")

    if(EUZEBIA3D_BUILD_APP)
        add_executable(Euzebia3D_PC "${EUZEBIA3D_WINDOWS_MAIN}")
        target_include_directories(Euzebia3D_PC PRIVATE
                "${CMAKE_CURRENT_LIST_DIR}"
                "${CMAKE_CURRENT_LIST_DIR}/storage"
        )
        target_compile_definitions(Euzebia3D_PC PRIVATE EUZEBIA3D_PLATFORM_WINDOWS=1)
        target_link_libraries(Euzebia3D_PC PRIVATE Euzebia3D_libs)
    endif()
endif()

