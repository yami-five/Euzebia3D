#include <stdint.h>
#include <stdio.h>

#if defined(EUZEBIA3D_PLATFORM_PICO)
#include "pico/multicore.h"
#elif defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#else
#error "Unsupported Euzebia3D platform"
#endif

#include "ICameraFactory.h"
#if defined(EUZEBIA3D_DEBUG_MODE)
#include "IDebugMode.h"
#endif
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
#if defined(EUZEBIA3D_DEBUG_MODE)
#include "debugMode.h"
#endif
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
#if defined(EUZEBIA3D_DEBUG_MODE)
static const IDebugMode *debugMode;
#endif

#define EUZEBIA3D_PLASMA_COLORS_NUM 16

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#define EUZEBIA3D_WINDOWS_TARGET_FPS 24u
#define EUZEBIA3D_PLASMA_SCALE 1
#define EUZEBIA3D_PLASMA_FAC_A 7
#define EUZEBIA3D_PLASMA_FAC_B 7
#define EUZEBIA3D_PLASMA_FAC_C 8
#define EUZEBIA3D_PLASMA_FAC_D 7
#define EUZEBIA3D_REQUIRE_POINTER(pointer, name) \
    do                                           \
    {                                            \
        if (!require_pointer((pointer), (name))) \
        {                                        \
            return 1;                            \
        }                                        \
    } while (0)
#else
#define EUZEBIA3D_PLASMA_SCALE 2
#define EUZEBIA3D_PLASMA_FAC_A 6
#define EUZEBIA3D_PLASMA_FAC_B 6
#define EUZEBIA3D_PLASMA_FAC_C 7
#define EUZEBIA3D_PLASMA_FAC_D 6
#define EUZEBIA3D_REQUIRE_POINTER(pointer, name) ((void)0)
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
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
    set_sys_clock_khz(320000, true);

    hardware_core = get_hardware();
    hardware_core->init_hardware();

    display = get_display();
    display->init_display(hardware_core);
#endif

    storage = get_storage();
    EUZEBIA3D_REQUIRE_POINTER(storage, "get_storage");

    painter = get_painter();
    EUZEBIA3D_REQUIRE_POINTER(painter, "get_painter");
    painter->init_painter(display, hardware_core, storage);

#if defined(EUZEBIA3D_DEBUG_MODE)
    debugMode = get_debugMode();
    EUZEBIA3D_REQUIRE_POINTER(debugMode, "get_debugMode");
    debugMode->init_debug_mode(hardware_core, painter);
#endif

    puppeteer = get_puppeteer();
    puppeteer->init_puppeteer(storage, painter);
    Puppet *pogodynka = puppeteer->create_puppet(0);

    renderer = get_renderer();
    EUZEBIA3D_REQUIRE_POINTER(renderer, "get_renderer");
    renderer->init_renderer(hardware_core, painter);
    renderer->set_scale(1);

    meshFactory = get_meshFactory();
    EUZEBIA3D_REQUIRE_POINTER(meshFactory, "get_meshFactory");
    meshFactory->init_mesh_factory(storage);

    Mesh *mug = meshFactory->create_textured_mesh(0, 1);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.3f, MODEL_TRANSFORM_TRANSLATE);

    Mesh *room = meshFactory->create_textured_mesh(1, 2);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0.2f, 0.0f, 1.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0, 2.2f, 2.2f, 2.2f, MODEL_TRANSFORM_SCALE);

    lightFactory = get_lightFactory();
    EUZEBIA3D_REQUIRE_POINTER(lightFactory, "get_lightFactory");
    PointLight *pointLight = lightFactory->create_point_light(10.0f, 10.0f, 0.0f, 15.0f, 0xffff);

    cameraFactory = get_cameraFactory();
    EUZEBIA3D_REQUIRE_POINTER(cameraFactory, "get_cameraFactory");
    Camera *camera = cameraFactory->create_camera(0.0f, 50.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    camera->transformations = add_camera_transformation(camera->transformations, &camera->transformationsNum, 0.0f, 0.0f, 0.0f, 0.0f, CAMERA_TRANSFORM_ROTATE);

    painter->clear_buffer(0x1100);
    painter->draw_buffer();

    uint32_t t = 0;

    uint16_t plasmaColors[EUZEBIA3D_PLASMA_COLORS_NUM] = {
        0x1be6,
        0x2427,
        0x3447,
        0x4488,
        0x54c8,
        0x5d09,
        0x6d49,
        0x7d8a,
        0x7d8a,
        0x6d49,
        0x5d09,
        0x54c8,
        0x4488,
        0x3447,
        0x2427,
        0x1be6,
    };
    Rectangle plasmaRect = {
        .x = 28,
        .y = 44,
        .height = 181,
        .width = 241,
    };
    Rectangle bar1 = {
        .x = 0,
        .y = 0,
        .height = 6,
        .width = 320,
    };
    Point lineStart = {
        .x = 0,
        .y = 0,
    };
    Point lineEnd = {
        .x = 100,
        .y = 100,
    };
    int running = 1;
    while (running)
    {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
        uint64_t frame_begin_ticks = SDL_GetPerformanceCounter();
#endif
#if defined(EUZEBIA3D_DEBUG_MODE)
        debugMode->begin_frame();
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
        running = process_window_events();
#endif
        float qt = t * 0.02f;
        (void)qt;
        // modify_mesh_transformation(room->transformations, qt, 0.0f, -10.0f, 0.0f, 0);
        // modify_mesh_transformation(mug->transformations, qt, 10.0f, -10.0f, 10.0f, 0);
        // update_camera(camera);
        // modify_camera_transformation(camera->transformations, 0.00f, 0.0f, 1.0f, 0.0f, 0);
        // renderer->clean_scene();
        // renderer->add_model_to_scene(room, camera, pointLight);
        // renderer->add_model_to_scene(mug, camera, pointLight);
        // renderer->render_scene(pointLight);
        // puppeteer->perform(pogodynka, t);

        /*painter->draw_plasma(
            plasmaColors,
            EUZEBIA3D_PLASMA_COLORS_NUM,
            t,
            EUZEBIA3D_PLASMA_SCALE,
            EUZEBIA3D_PLASMA_FAC_A,
            EUZEBIA3D_PLASMA_FAC_B,
            EUZEBIA3D_PLASMA_FAC_C,
            EUZEBIA3D_PLASMA_FAC_D,
            &plasmaRect);
        painter->print("test", 0, 20, 1, 0xffff);
        painter->draw_rectangle(&bar1, 0x34b2);
        painter->draw_line(&lineStart, &lineEnd, 0xfafa);*/

#if defined(EUZEBIA3D_DEBUG_MODE)
        debugMode->show_info();
        debugMode->begin_draw_buffer();
#endif
        painter->draw_buffer();
#if defined(EUZEBIA3D_DEBUG_MODE)
        debugMode->end_draw_buffer();
#endif
        t++;
        painter->clear_buffer(10);
#if defined(EUZEBIA3D_DEBUG_MODE)
        debugMode->end_frame();
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
        cap_window_frame_rate(frame_begin_ticks);
#endif
    }

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    SDL_Quit();
#endif

    return 0;
}
