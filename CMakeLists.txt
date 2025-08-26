# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
if (EXISTS ${picoVscode})
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

# Add executable. Default name is the project name, version 0.1

add_executable(Euzebia3D Euzebia3D.c )

pico_set_program_name(Euzebia3D "Euzebia3D")
pico_set_program_version(Euzebia3D "0.1")

# Generate PIO header
pico_generate_pio_header(Euzebia3D ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Euzebia3D 1)
pico_enable_stdio_usb(Euzebia3D 1)

# Add mine libraries
add_subdirectory(libs)

# Add the standard library to the build
target_link_libraries(Euzebia3D
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Euzebia3D PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
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

