#include <stdio.h>
#include "pico/multicore.h"

#include "ICameraFactory.h"
#include "IDisplay.h"
#include "IFileReader.h"
#include "IHardware.h"
#include "ILightFactory.h"
#include "IMeshFactory.h"
#include "IPainter.h"
#include "IRenderer.h"
#include "IPuppetFactory.h"

#include "cameraFactory.h"
#include "display.h"
#include "fileReader.h"
#include "hardware.h"
#include "lightFactory.h"
#include "meshFactory.h"
#include "painter.h"
#include "renderer.h"
#include "mesh.h"
#include "puppetFactory.h"
#include "puppet.h"

static const IHardware *hardware_core;
static const IDisplay *display;
static const IPainter *painter;
static const IFileReader *fileReader;
static const IRenderer *renderer;
static const IMeshFactory *meshFactory;
static const ILightFactory *lightFactory;
static const ICameraFactory *cameraFactory;
static const IPuppetFactory *puppetFactory;

void core1_main();

int main()
{
    set_sys_clock_khz(300000, true);

    hardware_core = get_hardware();
    hardware_core->init_hardware();

    display = get_display();
    display->init_display(hardware_core);

    painter = get_painter();
    painter->init_painter(display, hardware_core);

    renderer = get_renderer();
    renderer->init_renderer(hardware_core, painter);

    meshFactory = get_meshFactory();
    Mesh *pizza = meshFactory->create_textured_mesh(7, 7);
    pizza->transformations = add_transformation(pizza->transformations, &pizza->transformationsNum, 0, 10.0f, 10.0f, 10.0f, 0);

    lightFactory = get_lightFactory();
    PointLight *pointLight = lightFactory->create_point_light(0.0f, 0.0f, 3.0f, 1.0f, 0xffff);

    cameraFactory = get_cameraFactory();
    Camera *camera = cameraFactory->create_camera(0.0f, 0.0f, 25.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    painter->clear_buffer(0x1100);
    painter->draw_buffer();
    uint32_t t = 0;

    while (1)
    {
        float qt = t * 0.2f;
        modify_transformation(pizza->transformations, -qt, 10.0f, 0.0f, 0.0f, 0);
        renderer->draw_model(pizza, pointLight, camera);
        // painter->apply_post_process_effect(0);
        painter->draw_buffer();
        t++;
        renderer->clear_zbuffer();
        painter->clear_buffer(0);
        // painter->clear_buffer(0x11);
    }
    // multicore_launch_core1(core1_main);
}

void core1_main()
{
    hardware_core->init_audio_i2s();
    fileReader = get_fileReader();
    fileReader->init_fileReader(hardware_core);
    fileReader->play_wave_file("kostek.wav");
    fileReader->play_wave_file("kosteke.wav");
}