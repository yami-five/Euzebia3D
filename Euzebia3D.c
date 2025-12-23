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
    renderer->set_scale(1);

    meshFactory = get_meshFactory();
    Mesh *triangle1 = meshFactory->create_colored_mesh(0x1234, 0);
    triangle1->transformations = add_transformation(triangle1->transformations, &triangle1->transformationsNum, 0, 10.0f, 10.0f, 10.0f, 0);

    Mesh *triangle2 = meshFactory->create_colored_mesh(0x9999, 0);
    triangle2->transformations = add_transformation(triangle2->transformations, &triangle2->transformationsNum, 0, 10.0f, 10.0f, 10.0f, 0);
    triangle2->transformations = add_transformation(triangle2->transformations, &triangle2->transformationsNum, 0, 0.0f, 0.0f, 0.2f, 1);

    Mesh *mug = meshFactory->create_textured_mesh(0, 1);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 10.0f, 10.0f, 10.0f, 0);

    lightFactory = get_lightFactory();
    PointLight *pointLight = lightFactory->create_point_light(-1.0f, 0.0f, 1.0f, 5.0f, 0xffff);

    cameraFactory = get_cameraFactory();
    Camera *camera = cameraFactory->create_camera(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    painter->clear_buffer(0x1100);
    painter->draw_buffer();
    uint32_t t = 0;

    while (1)
    {
        float qt = t * 0.02f;
        modify_transformation(mug->transformations, qt, 10.0f, 10.0f, 10.0f, 0);
        renderer->draw_model(mug, pointLight, camera);
        // painter->apply_post_process_effect(0);
        painter->draw_buffer();
        t++;
        renderer->clear_zbuffer();
        painter->clear_buffer(10);
        // sleep_ms(2000);
        // painter->clear_buffer(0x11);
    }
    // multicore_launch_core1(core1_main);
}
