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
#include "IStorage.h"

#include "cameraFactory.h"
#include "display.h"
#include "fileReader.h"
#include "hardware.h"
#include "lightFactory.h"
#include "meshFactory.h"
#include "painter.h"
#include "renderer.h"
#include "mesh.h"
#include "camera.h"
#include "puppetFactory.h"
#include "puppet.h"
#include "storage.h"

static const IHardware *hardware_core;
static const IDisplay *display;
static const IPainter *painter;
static const IFileReader *fileReader;
static const IRenderer *renderer;
static const IMeshFactory *meshFactory;
static const ILightFactory *lightFactory;
static const ICameraFactory *cameraFactory;
static const IPuppetFactory *puppetFactory;
static const IStorage *storage;

void core1_main();

int main()
{
    set_sys_clock_khz(300000, true);

    hardware_core = get_hardware();
    hardware_core->init_hardware();

    display = get_display();
    display->init_display(hardware_core);

    storage = get_storage();

    painter = get_painter();
    painter->init_painter(display, hardware_core, storage);

    renderer = get_renderer();
    renderer->init_renderer(hardware_core, painter);
    renderer->set_scale(1);

    meshFactory = get_meshFactory();
    meshFactory->init_mesh_factory(storage);

    Mesh *mug = meshFactory->create_textured_mesh(0, 1);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.0f, 0);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.3f, 1);

    Mesh *room = meshFactory->create_textured_mesh(1, 2);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0.2f, 0.0f, 1.0f, 0.0f, 0);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0, 2.2f, 2.2f, 2.2f, 2);

    lightFactory = get_lightFactory();
    PointLight *pointLight = lightFactory->create_point_light(10.0f, 10.0f, 0.0f, 15.0f, 0xffff);

    cameraFactory = get_cameraFactory();
    Camera *camera = cameraFactory->create_camera(0.0f, 50.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    camera->transformations = add_camera_transformation(camera->transformations, &camera->transformationsNum, 0.0f, 0.0f, 0.0f, 0.0f, CAMERA_TRANSFORM_ROTATE);

    painter->clear_buffer(0x1100);
    painter->draw_buffer();
    uint32_t t = 0;

    const uint8_t sprite_size = 16;
    const Sprite *left_edge = storage->get_sprite(0);
    const Sprite *right_edge = storage->get_sprite(1);
    const Sprite *top_edge = storage->get_sprite(2);
    const Sprite *bottom_edge = storage->get_sprite(3);

    while (1)
    {
        float qt = t * 0.02f;
        // modify_mesh_transformation(room->transformations, qt, 0.0f, 10.0f, 0.0f, 0);
        modify_mesh_transformation(mug->transformations, qt, 10.0f, 10.0f, 10.0f, 0);
        update_camera(camera);
        modify_camera_transformation(camera->transformations, 0.00f, 0.0f, 1.0f, 0.0f, 0);
        renderer->clean_scene();
        // renderer->add_model_to_scene(room, camera, pointLight);
        renderer->add_model_to_scene(mug, camera, pointLight);
        renderer->render_scene(pointLight);
        // painter->apply_post_process_effect(0);

        // painter->draw_sprite(left_edge,0,120-(sprite_size>>1),0,1);
        // painter->draw_sprite(right_edge,320-sprite_size,120-(sprite_size>>1),0,1);
        // painter->draw_sprite(top_edge,160-(sprite_size>>1),0,0,1);
        // painter->draw_sprite(bottom_edge,160-(sprite_size>>1),240-sprite_size,0,1);

        painter->draw_buffer();
        t++;
        painter->clear_buffer(10);
        // sleep_ms(2000);
        // painter->clear_buffer(0x11);
    }
    // multicore_launch_core1(core1_main);
}
