add_library(Euzebia3D_PicoDeps INTERFACE)
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
target_link_libraries(Euzebia3D_CameraFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics Euzebia3D_Transformations)

add_library(
    Euzebia3D_Display
    ${CMAKE_CURRENT_SOURCE_DIR}/display/display.c
)
target_include_directories(
    Euzebia3D_Display
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/display
)
target_link_libraries(Euzebia3D_Display PUBLIC Euzebia3D_PicoDeps Euzebia3D_Hardware)

add_library(
    Euzebia3D_FileReader
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fileReader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ffsystem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ffunicode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/sd_driver/sd_driver.c
)
target_include_directories(
    Euzebia3D_FileReader
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/sd_driver
)
target_link_libraries(Euzebia3D_FileReader PUBLIC Euzebia3D_PicoDeps)
target_link_libraries(Euzebia3D_FileReader PUBLIC Euzebia3D_Painter Euzebia3D_Hardware)

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
pico_generate_pio_header(Euzebia3D_Hardware ${CMAKE_CURRENT_LIST_DIR}/hardware/audio_pio.pio)

add_library(
    Euzebia3D_Storage
    ${CMAKE_SOURCE_DIR}/storage/storage.c
)
target_include_directories(
    Euzebia3D_Storage
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
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
    Euzebia3D_MeshFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/mesh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/meshFactory.c
)
target_include_directories(
    Euzebia3D_MeshFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory
)
target_link_libraries(Euzebia3D_MeshFactory PUBLIC Euzebia3D_PicoDeps Euzebia3D_Arithmetics Euzebia3D_Storage Euzebia3D_Transformations)

add_library(
    Euzebia3D_Painter
    ${CMAKE_CURRENT_SOURCE_DIR}/painter/painter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/painter/gradient.c
)
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
    Euzebia3D_PuppetFactory
)

add_library(
    Euzebia3D_PuppetFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/animation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/puppet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/puppetFactory.c
)
target_include_directories(
    Euzebia3D_PuppetFactory
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory
)
target_link_libraries(
    Euzebia3D_PuppetFactory
    PUBLIC
    Euzebia3D_PicoDeps
    Euzebia3D_Arithmetics
    Euzebia3D_MeshFactory
    Euzebia3D_Storage
)

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

add_library(
    Euzebia3D_Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/renderer.c
)
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
    Euzebia3D_MeshFactory
    Euzebia3D_LightFactory
    Euzebia3D_Painter
    Euzebia3D_CameraFactory
)

add_library(Euzebia3D_libs INTERFACE)
target_link_libraries(
    Euzebia3D_libs
    INTERFACE
    Euzebia3D_Arithmetics
    Euzebia3D_CameraFactory
    Euzebia3D_Display
    Euzebia3D_FileReader
    Euzebia3D_Hardware
    Euzebia3D_LightFactory
    Euzebia3D_MeshFactory
    Euzebia3D_Painter
    Euzebia3D_PuppetFactory
    Euzebia3D_Renderer
    Euzebia3D_Storage
    Euzebia3D_Transformations
)

add_library(Euzebia3D_CameraFactoryInterface INTERFACE)
add_library(Euzebia3D_DisplayInterface INTERFACE)
add_library(Euzebia3D_FileReaderInterface INTERFACE)
add_library(Euzebia3D_HardwareInterface INTERFACE)
add_library(Euzebia3D_LightFactoryInterface INTERFACE)
add_library(Euzebia3D_MeshFactoryInterface INTERFACE)
add_library(Euzebia3D_PainterInterface INTERFACE)
add_library(Euzebia3D_PuppetFactoryInterface INTERFACE)
add_library(Euzebia3D_RendererInterface INTERFACE)
add_library(Euzebia3D_StorageInterface INTERFACE)

target_link_libraries(Euzebia3D_CameraFactoryInterface INTERFACE Euzebia3D_CameraFactory)
target_link_libraries(Euzebia3D_DisplayInterface INTERFACE Euzebia3D_Display)
target_link_libraries(Euzebia3D_FileReaderInterface INTERFACE Euzebia3D_FileReader)
target_link_libraries(Euzebia3D_HardwareInterface INTERFACE Euzebia3D_Hardware)
target_link_libraries(Euzebia3D_LightFactoryInterface INTERFACE Euzebia3D_LightFactory)
target_link_libraries(Euzebia3D_MeshFactoryInterface INTERFACE Euzebia3D_MeshFactory)
target_link_libraries(Euzebia3D_PainterInterface INTERFACE Euzebia3D_Painter)
target_link_libraries(Euzebia3D_PuppetFactoryInterface INTERFACE Euzebia3D_PuppetFactory)
target_link_libraries(Euzebia3D_RendererInterface INTERFACE Euzebia3D_Renderer)
target_link_libraries(Euzebia3D_StorageInterface INTERFACE Euzebia3D_Storage)
