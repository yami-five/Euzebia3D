add_library(
    Euzebia3D 
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/fpa.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/vectors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics/lookup_tables.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory/camera.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory/cameraFactory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/display/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fileReader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ffsystem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs/ffunicode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/sd_driver/sd_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/hardware.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory/light.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory/lightFactory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/mesh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory/meshFactory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/painter/painter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/painter/gradient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/animation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/puppet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory/puppetFactory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/renderer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/fonts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/gfx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/post_processing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/scrollers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/sprites.c
    ${CMAKE_CURRENT_SOURCE_DIR}/shared/rawPuppets.c
)

target_link_libraries(
    Euzebia3D
    PUBLIC 
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
pico_generate_pio_header(Euzebia3D ${CMAKE_CURRENT_LIST_DIR}/hardware/audio_pio.pio)

target_include_directories(Euzebia3D
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/arithmetics
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/display
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/painter
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/fatfs
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader/sd_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
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

target_include_directories(
    Euzebia3D_CameraFactoryInterface 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/cameraFactory  
)

target_include_directories(
    Euzebia3D_DisplayInterface 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/display  
)

target_include_directories(
    Euzebia3D_FileReaderInterface
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader  
)

target_include_directories(
    Euzebia3D_HardwareInterface
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware  
)

target_include_directories(
    Euzebia3D_LightFactoryInterface
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lightFactory
)

target_include_directories(
    Euzebia3D_MeshFactoryInterface
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/meshFactory
)

target_include_directories(
    Euzebia3D_PainterInterface 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/painter  
)

target_include_directories(
    Euzebia3D_PuppetFactoryInterface 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/puppetFactory  
)

target_include_directories(
    Euzebia3D_RendererInterface 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer  
)