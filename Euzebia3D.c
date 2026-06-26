#include <stdint.h>
#include <stdio.h>

#if defined(EUZEBIA3D_PLATFORM_PICO)
#include "pico/multicore.h"
#include "pico/time.h"
#elif defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#else
#error "Unsupported Euzebia3D platform"
#endif

#include "ICameraFactory.h"
#include "IDisplay.h"
#include "IHardware.h"
#include "ILightFactory.h"
#include "IMeshFactory.h"
#include "IPainter.h"
#include "IRenderer.h"
#include "IStorage.h"
#include "IPuppeteer.h"

#include "camera.h"
#include "cameraFactory.h"
#include "lightFactory.h"
#include "mesh.h"
#include "meshFactory.h"
#include "renderer.h"
#include "storage.h"
#include "puppet.h"
#include "puppeteer.h"

#if defined(EUZEBIA3D_PLATFORM_PICO)
#include "display.h"
#include "hardware.h"
#endif

const IPainter *get_painter(void);

static const IHardware *hardware_core;
static const IDisplay *display;
static const IPainter *painter;
static const IRenderer *renderer;
static const IMeshFactory *meshFactory;
static const ILightFactory *lightFactory;
static const ICameraFactory *cameraFactory;
static const IStorage *storage;
static const IPuppeteer *puppeteer;

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#define EUZEBIA3D_WINDOWS_TARGET_FPS 24u

static int require_pointer(const void *pointer, const char *name)
{
    if (pointer != NULL)
    {
        return 1;
    }

    SDL_Log("%s failed", name);
    return 0;
}

static int process_window_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            return 0;
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
        {
            return 0;
        }
    }

    return 1;
}

static void cap_window_frame_rate(uint64_t frame_begin_ticks)
{
    uint64_t performance_frequency = SDL_GetPerformanceFrequency();
    if (performance_frequency == 0u || EUZEBIA3D_WINDOWS_TARGET_FPS == 0u)
    {
        return;
    }

    uint64_t target_frame_ticks = performance_frequency / EUZEBIA3D_WINDOWS_TARGET_FPS;
    uint64_t elapsed_ticks = SDL_GetPerformanceCounter() - frame_begin_ticks;
    if (elapsed_ticks >= target_frame_ticks)
    {
        return;
    }

    uint64_t remaining_ticks = target_frame_ticks - elapsed_ticks;
    uint64_t remaining_ms = (remaining_ticks * 1000u) / performance_frequency;
    if (remaining_ms > 0u)
    {
        SDL_Delay((uint32_t)remaining_ms);
    }
}
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
int main(int argc, char **argv)
#else
int main(void)
#endif
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)argc;
    (void)argv;
#endif

#if defined(EUZEBIA3D_PLATFORM_PICO)
    set_sys_clock_khz(300000, true);

    hardware_core = get_hardware();
    hardware_core->init_hardware();

    display = get_display();
    display->init_display(hardware_core);
#endif

    storage = get_storage();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(storage, "get_storage"))
    {
        return 1;
    }
#endif

    painter = get_painter();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(painter, "get_painter"))
    {
        return 1;
    }
#endif
    painter->init_painter(display, hardware_core, storage);

    puppeteer = get_puppeteer();
    puppeteer->init_puppeteer(storage, painter);
    Puppet *pogodynka = puppeteer->create_puppet(0);

    renderer = get_renderer();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(renderer, "get_renderer"))
    {
        return 1;
    }
#endif
    renderer->init_renderer(hardware_core, painter);
    renderer->set_scale(1);

    meshFactory = get_meshFactory();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(meshFactory, "get_meshFactory"))
    {
        return 1;
    }
#endif
    meshFactory->init_mesh_factory(storage);

    Mesh *mug = meshFactory->create_textured_mesh(0, 1);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.3f, MODEL_TRANSFORM_TRANSLATE);

    Mesh *room = meshFactory->create_textured_mesh(1, 2);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0.2f, 0.0f, 1.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0, 2.2f, 2.2f, 2.2f, MODEL_TRANSFORM_SCALE);

    lightFactory = get_lightFactory();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(lightFactory, "get_lightFactory"))
    {
        return 1;
    }
#endif
    PointLight *pointLight = lightFactory->create_point_light(10.0f, 10.0f, 0.0f, 15.0f, 0xffff);

    cameraFactory = get_cameraFactory();
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!require_pointer(cameraFactory, "get_cameraFactory"))
    {
        return 1;
    }
#endif
    Camera *camera = cameraFactory->create_camera(0.0f, 50.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    camera->transformations = add_camera_transformation(camera->transformations, &camera->transformationsNum, 0.0f, 0.0f, 0.0f, 0.0f, CAMERA_TRANSFORM_ROTATE);

    painter->clear_buffer(0x1100);
    painter->draw_buffer();

    uint32_t t = 0;

    uint16_t plasmaColors[15] = {
        0x1be6,
        0x2427,
        0x3447,
        0x4488,
        0x54c8,
        0x5d09,
        0x6d49,
        0x7d8a,
        0x6d49,
        0x5d09,
        0x54c8,
        0x4488,
        0x3447,
        0x2427,
        0x1be6,
    };
    Rectangle plasmaRect={
        .x=0,
        .y=0,
        .height=150,
        .width=150,
    };
    Rectangle bar1 = {
        .x=0,
        .y=0,
        .height=6,
        .width=320,
    };
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    int running = 1;
    while (running)
    {
        uint64_t frame_begin_ticks = SDL_GetPerformanceCounter();

        running = process_window_events();

        float qt = t * 0.002f;
        // modify_mesh_transformation(room->transformations, qt, 0.0f, -10.0f, 0.0f, 0);
        // modify_mesh_transformation(mug->transformations, qt, 10.0f, -10.0f, 10.0f, 0);
        // update_camera(camera);
        // modify_camera_transformation(camera->transformations, 0.00f, 0.0f, 1.0f, 0.0f, 0);
        // renderer->clean_scene();
        // renderer->add_model_to_scene(room, camera, pointLight);
        // renderer->add_model_to_scene(mug, camera, pointLight);
        // renderer->render_scene(pointLight);
        //  puppeteer->perform(pogodynka, t);
        painter->draw_plasma(plasmaColors, 15, t, 7, 7, 8, 7, &plasmaRect);
        painter->print("test", 0, 20, 1, 0xffff);
        painter->draw_rectangle(&bar1, 0x34b2);
        painter->draw_buffer();
        t++;
        painter->clear_buffer(10);

        cap_window_frame_rate(frame_begin_ticks);
    }

    SDL_Quit();
    return 0;
#else
    while (1)
    {
        uint64_t frame_begin_us = time_us_64();
        (void)frame_begin_us;

        // puppeteer->perform(pogodynka, t);
        painter->draw_plasma(plasmaColors, 15, t, 7, 7, 8, 7, &plasmaRect);
        painter->print("test",0,20,1,0xffff);

        painter->draw_buffer();
        t++;
        painter->clear_buffer(10);
    }

    return 0;
#endif
}
