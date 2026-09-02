if(NOT DEFINED EUZEBIA3D_PLATFORM_NORMALIZED)
    set(EUZEBIA3D_PLATFORM_NORMALIZED "PICO")
endif()

# The public variable inherited from the parent project always wins over an
# internal or cached normalized value.
if(DEFINED EUZEBIA3D_PLATFORM)
    string(TOUPPER "${EUZEBIA3D_PLATFORM}" EUZEBIA3D_PLATFORM_NORMALIZED)
endif()

if(NOT DEFINED EUZEBIA3D_PICO_DISPLAY)
    set(EUZEBIA3D_PICO_DISPLAY "ST7789")
endif()
string(TOUPPER "${EUZEBIA3D_PICO_DISPLAY}" EUZEBIA3D_PICO_DISPLAY_NORMALIZED)

if(NOT EUZEBIA3D_PLATFORM_NORMALIZED STREQUAL "PICO" AND NOT EUZEBIA3D_PLATFORM_NORMALIZED STREQUAL "WINDOWS")
    message(FATAL_ERROR "Unsupported EUZEBIA3D_PLATFORM_NORMALIZED='${EUZEBIA3D_PLATFORM_NORMALIZED}'")
endif()

if(NOT DEFINED EUZEBIA3D_DEBUG_STAGE_ENABLED)
    set(EUZEBIA3D_DEBUG_STAGE_ENABLED ON)
endif()
if(NOT DEFINED EUZEBIA3D_RENDERER_SHADING_ENABLED)
    set(EUZEBIA3D_RENDERER_SHADING_ENABLED ON)
endif()
if(NOT DEFINED EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED)
    set(EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED ON)
endif()
if(NOT DEFINED EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED)
    set(EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED ON)
endif()
if(NOT DEFINED EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED)
    set(EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED ON)
endif()

set(EUZEBIA3D_PLATFORM_WINDOWS OFF)
if(EUZEBIA3D_PLATFORM_NORMALIZED STREQUAL "WINDOWS")
    set(EUZEBIA3D_PLATFORM_WINDOWS ON)

    set(EUZEBIA3D_WINDOWS_WINDOW_WIDTH 960 CACHE STRING "Initial width of the Windows output window")
    set(EUZEBIA3D_WINDOWS_WINDOW_HEIGHT 720 CACHE STRING "Initial height of the Windows output window")
    option(EUZEBIA3D_WINDOWS_FULLSCREEN "Render the Windows output in fullscreen mode" OFF)

    if(NOT EUZEBIA3D_WINDOWS_WINDOW_WIDTH MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "EUZEBIA3D_WINDOWS_WINDOW_WIDTH must be a positive integer")
    endif()
    if(NOT EUZEBIA3D_WINDOWS_WINDOW_HEIGHT MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "EUZEBIA3D_WINDOWS_WINDOW_HEIGHT must be a positive integer")
    endif()
endif()

add_library(Euzebia3D_PlatformDefs INTERFACE)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    target_compile_definitions(Euzebia3D_PlatformDefs INTERFACE EUZEBIA3D_PLATFORM_WINDOWS=1)
else()
    target_compile_definitions(Euzebia3D_PlatformDefs INTERFACE EUZEBIA3D_PLATFORM_PICO=1)
endif()
if(EUZEBIA3D_USE_PSRAM)
    target_compile_definitions(Euzebia3D_PlatformDefs INTERFACE EUZEBIA3D_USE_PSRAM=1)
endif()
if(EUZEBIA3D_DEBUG_STAGE_ENABLED)
    target_compile_definitions(Euzebia3D_PlatformDefs INTERFACE EUZEBIA3D_DEBUG_STAGE_ENABLED=1)
endif()

add_library(Euzebia3D_PicoDeps INTERFACE)
target_link_libraries(Euzebia3D_PicoDeps INTERFACE Euzebia3D_PlatformDefs)
if(NOT EUZEBIA3D_PLATFORM_WINDOWS)
    target_link_libraries(
        Euzebia3D_PicoDeps
        INTERFACE
        pico_stdlib
        hardware_adc
        hardware_clocks
        hardware_dma
        hardware_i2c
        hardware_pio
        hardware_pwm
        hardware_spi
        pico_audio
        pico_audio_i2s
    )
    if(EUZEBIA3D_USE_PSRAM)
        target_link_libraries(
            Euzebia3D_PicoDeps
            INTERFACE
            hardware_flash
            hardware_xip_cache
        )
    endif()
endif()

set(EUZEBIA3D_STORAGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../storage")

set(EUZEBIA3D_AUDIO_PLAYER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/audioPlayer.c
)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    list(APPEND EUZEBIA3D_AUDIO_PLAYER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/windows/audioPlayer_platform_win.c
    )
else()
    list(APPEND EUZEBIA3D_AUDIO_PLAYER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/audioPlayer_platform_pico2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/fatfs/ff.c
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/fatfs/diskio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/fatfs/ffsystem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/fatfs/ffunicode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/sd_driver/sd_driver.c
    )
endif()
if(EUZEBIA3D_PLATFORM_WINDOWS)
    find_path(EUZEBIA3D_MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h)
    if(NOT EUZEBIA3D_MINIAUDIO_INCLUDE_DIR)
        message(FATAL_ERROR "miniaudio.h not found. Install miniaudio or pass -DEUZEBIA3D_MINIAUDIO_INCLUDE_DIR=<directory containing miniaudio.h>.")
    endif()
endif()

add_library(
    Euzebia3D_Arithmetics
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/fpa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/vectors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/lookup_tables.c
)
target_include_directories(
    Euzebia3D_Arithmetics
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics
)
target_link_libraries(Euzebia3D_Arithmetics PUBLIC Euzebia3D_PicoDeps)

add_library(
    Euzebia3D_CameraFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory/camera.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory/cameraFactory.c
)
target_include_directories(
    Euzebia3D_CameraFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory
)
target_link_libraries(Euzebia3D_CameraFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics)

set(EUZEBIA3D_DISPLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/display/display.c
)

if(EUZEBIA3D_PLATFORM_WINDOWS)
    list(APPEND EUZEBIA3D_DISPLAY_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/display/win_display/win_display.c
    )
elseif(EUZEBIA3D_PICO_DISPLAY_NORMALIZED STREQUAL "ST7789")
    list(APPEND EUZEBIA3D_DISPLAY_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/display/st7789/st7789.c
    )
elseif(EUZEBIA3D_PICO_DISPLAY_NORMALIZED STREQUAL "DVI")
    set(EUZEBIA3D_DVI_DISPLAY_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/display/dvi/dvi.c
    )
    if(NOT EXISTS "${EUZEBIA3D_DVI_DISPLAY_SOURCE}")
        message(FATAL_ERROR
            "DVI display backend selected, but its implementation is missing: ${EUZEBIA3D_DVI_DISPLAY_SOURCE}"
        )
    endif()
    list(APPEND EUZEBIA3D_DISPLAY_SOURCES ${EUZEBIA3D_DVI_DISPLAY_SOURCE})
else()
    message(FATAL_ERROR
        "Unsupported EUZEBIA3D_PICO_DISPLAY='${EUZEBIA3D_PICO_DISPLAY}'"
    )
endif()

add_library(Euzebia3D_Display ${EUZEBIA3D_DISPLAY_SOURCES})

target_include_directories(
    Euzebia3D_Display
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/display
)
target_link_libraries(Euzebia3D_Display PUBLIC Euzebia3D_PicoDeps Euzebia3D_Hardware)

add_library(
    Euzebia3D_AudioPlayer
    ${EUZEBIA3D_AUDIO_PLAYER_SOURCES}
)
target_include_directories(
    Euzebia3D_AudioPlayer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_player
)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    target_include_directories(Euzebia3D_AudioPlayer PRIVATE ${EUZEBIA3D_MINIAUDIO_INCLUDE_DIR})
else()
    target_include_directories(
        Euzebia3D_AudioPlayer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/fatfs
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_player/rpi_pico2/sd_driver
    )
endif()
target_link_libraries(Euzebia3D_AudioPlayer PUBLIC Euzebia3D_PicoDeps Euzebia3D_Painter Euzebia3D_Hardware)

add_library(
    Euzebia3D_Hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.c
)
target_include_directories(
    Euzebia3D_Hardware
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
)
target_link_libraries(Euzebia3D_Hardware PUBLIC Euzebia3D_PicoDeps)
if(NOT EUZEBIA3D_PLATFORM_WINDOWS)
    pico_generate_pio_header(Euzebia3D_Hardware ${CMAKE_CURRENT_LIST_DIR}/hardware/audio_pio.pio)
endif()

add_library(
    Euzebia3D_Storage
    ${EUZEBIA3D_STORAGE_DIR}/storage.c
)
target_include_directories(
    Euzebia3D_Storage
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
    ${EUZEBIA3D_STORAGE_DIR}
)
target_link_libraries(Euzebia3D_Storage PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics)

add_library(
    Euzebia3D_LightFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory/light.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory/lightFactory.c
)
target_include_directories(
    Euzebia3D_LightFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory
)
target_link_libraries(Euzebia3D_LightFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics)

add_library(
    Euzebia3D_MaterialFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/materialFactory/material.c
    ${CMAKE_CURRENT_SOURCE_DIR}/materialFactory/materialFactory.c
)
target_include_directories(
    Euzebia3D_MaterialFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/materialFactory
)
target_link_libraries(Euzebia3D_MaterialFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics Euzebia3D_Storage)

add_library(
    Euzebia3D_MeshFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/mesh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/meshFactory.c
)
target_include_directories(
    Euzebia3D_MeshFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory
)
target_link_libraries(Euzebia3D_MeshFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics Euzebia3D_Storage Euzebia3D_Transformations Euzebia3D_MaterialFactory)

set(EUZEBIA3D_PAINTER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/painter/painter.c
)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    list(APPEND EUZEBIA3D_PAINTER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/painter/windows/painter_platform_win.c
    )
else()
    list(APPEND EUZEBIA3D_PAINTER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/painter/rpi_pico2/painter_platform_pico2.c
    )
endif()

add_library(Euzebia3D_Painter ${EUZEBIA3D_PAINTER_SOURCES})
target_include_directories(
    Euzebia3D_Painter
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/painter
)
target_link_libraries(
    Euzebia3D_Painter
    PUBLIC
    Euzebia3D_PicoDeps
    Euzebia3D_Display
    Euzebia3D_Hardware
    Euzebia3D_Arithmetics
    Euzebia3D_Storage
    Euzebia3D_Puppeteer
)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    target_link_libraries(Euzebia3D_Painter PUBLIC SDL3::SDL3)
    target_compile_definitions(
        Euzebia3D_Painter
        PRIVATE
        EUZEBIA3D_WINDOWS_WINDOW_WIDTH=${EUZEBIA3D_WINDOWS_WINDOW_WIDTH}
        EUZEBIA3D_WINDOWS_WINDOW_HEIGHT=${EUZEBIA3D_WINDOWS_WINDOW_HEIGHT}
    )
    if(EUZEBIA3D_WINDOWS_FULLSCREEN)
        target_compile_definitions(Euzebia3D_Painter PRIVATE EUZEBIA3D_WINDOWS_FULLSCREEN=1)
    else()
        target_compile_definitions(Euzebia3D_Painter PRIVATE EUZEBIA3D_WINDOWS_FULLSCREEN=0)
    endif()
endif()

add_library(
    Euzebia3D_DebugMode
    ${CMAKE_CURRENT_SOURCE_DIR}/debugMode/debugMode.c
)
target_include_directories(
    Euzebia3D_DebugMode
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/debugMode
)
target_link_libraries(
    Euzebia3D_DebugMode
    PUBLIC
    Euzebia3D_PicoDeps
    Euzebia3D_Hardware
    Euzebia3D_Painter
)

add_library(
    Euzebia3D_Puppeteer
    ${CMAKE_CURRENT_SOURCE_DIR}/puppeteer/puppeteer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppeteer/puppetCalc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppeteer/puppetFactory.c
)
target_include_directories(
    Euzebia3D_Puppeteer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/puppeteer
)
target_link_libraries(
    Euzebia3D_Puppeteer
    PUBLIC
    Euzebia3D_PicoDeps
    Euzebia3D_Arithmetics
    Euzebia3D_Storage
    Euzebia3D_Painter
)
if(NOT EUZEBIA3D_PLATFORM_WINDOWS)
    target_link_libraries(Euzebia3D_Puppeteer PUBLIC hardware_interp)
endif()

add_library(
    Euzebia3D_Transformations
    ${CMAKE_CURRENT_SOURCE_DIR}/transformations/transformations.c
)
target_include_directories(
    Euzebia3D_Transformations
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transformations
)
target_link_libraries(Euzebia3D_Transformations PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics)

set(EUZEBIA3D_RENDERER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/renderer.c
)
if(EUZEBIA3D_PLATFORM_WINDOWS)
    list(APPEND EUZEBIA3D_RENDERER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer/windows/renderer_platform_win.c
    )
else()
    list(APPEND EUZEBIA3D_RENDERER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer/rpi_pico2/renderer_platform_pico2.c
    )
endif()

add_library(Euzebia3D_Renderer ${EUZEBIA3D_RENDERER_SOURCES})
target_include_directories(
    Euzebia3D_Renderer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer
)
target_link_libraries(
    Euzebia3D_Renderer
    PUBLIC
    Euzebia3D_PicoDeps
    Euzebia3D_Arithmetics
    Euzebia3D_Transformations
    Euzebia3D_MaterialFactory
    Euzebia3D_MeshFactory
    Euzebia3D_LightFactory
    Euzebia3D_Painter
    Euzebia3D_CameraFactory
)
if(EUZEBIA3D_TEXTURE_CACHE_ENABLED)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_ENABLED=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_ENABLED=0)
endif()
if(EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH=0)
endif()
if(EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM=0)
endif()
if(EUZEBIA3D_RENDERER_SHADING_ENABLED)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_SHADING_ENABLED=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_SHADING_ENABLED=0)
endif()
if(EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED=0)
endif()
if(EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED=0)
endif()
if(EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED)
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED=1)
else()
    target_compile_definitions(Euzebia3D_Renderer PRIVATE EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED=0)
endif()
if(NOT EUZEBIA3D_PLATFORM_WINDOWS)
    target_link_libraries(Euzebia3D_Renderer PRIVATE hardware_interp)
endif()

add_library(
    Euzebia3D_EngineApi
    ${CMAKE_CURRENT_SOURCE_DIR}/engine_api/engineApi.c
)

target_include_directories(
    Euzebia3D_EngineApi
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine_api
)

target_link_libraries(
    Euzebia3D_EngineApi
    PUBLIC
    Euzebia3D_AudioPlayer
    Euzebia3D_CameraFactory
    Euzebia3D_DebugMode
    Euzebia3D_Display
    Euzebia3D_Hardware
    Euzebia3D_LightFactory
    Euzebia3D_MaterialFactory
    Euzebia3D_MeshFactory
    Euzebia3D_Painter
    Euzebia3D_Puppeteer
    Euzebia3D_Renderer
    Euzebia3D_Storage
)

add_library(Euzebia3D_libs INTERFACE)
target_link_libraries(
    Euzebia3D_libs
    INTERFACE
    Euzebia3D_EngineApi
)
