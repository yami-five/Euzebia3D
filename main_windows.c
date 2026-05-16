#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "IPainter.h"
#include "storage.h"
#include "IStorage.h"
#include "IRenderer.h"
#include "IMeshFactory.h"
#include "ILightFactory.h"
#include "ICameraFactory.h"
#include "IPuppetFactory.h"
#include "renderer.h"
#include "meshFactory.h"
#include "lightFactory.h"
#include "cameraFactory.h"
#include "puppetFactory.h"

const IPainter *get_painter(void);

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    const IStorage *storage = get_storage();
    if (storage == NULL) {
        SDL_Log("get_storage failed");
        return 1;
    }

    const IPainter *painter = get_painter();
    if (painter == NULL) {
        SDL_Log("get_painter failed");
        return 1;
    }
    painter->init_painter(NULL, NULL, storage);

    const IRenderer *renderer = get_renderer();
    if (renderer == NULL) {
        SDL_Log("get_renderer failed");
        return 1;
    }
    renderer->init_renderer(NULL, painter);
    renderer->set_scale(1);

    const IMeshFactory *meshFactory = get_meshFactory();
    if (meshFactory == NULL) {
        SDL_Log("get_meshFactory failed");
        return 1;
    }
    meshFactory->init_mesh_factory(storage);

    Mesh* mug = meshFactory->create_textured_mesh(0, 1);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    mug->transformations = add_transformation(mug->transformations, &mug->transformationsNum, 0, 0.0f, 0.0f, 0.3f, MODEL_TRANSFORM_TRANSLATE);

    Mesh* room = meshFactory->create_textured_mesh(1, 2);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0.2f, 0.0f, 1.0f, 0.0f, MODEL_TRANSFORM_ROTATE);
    room->transformations = add_transformation(room->transformations, &room->transformationsNum, 0, 2.2f, 2.2f, 2.2f, MODEL_TRANSFORM_SCALE);


    const ILightFactory *lightFactory = get_lightFactory();
    if (lightFactory == NULL) {
        SDL_Log("get_lightFactory failed");
        return 1;
    }
    PointLight* pointLight = lightFactory->create_point_light(10.0f, 10.0f, 0.0f, 15.0f, 0xffff);

    const ICameraFactory *cameraFactory = get_cameraFactory();
    if (cameraFactory == NULL) {
        SDL_Log("get_cameraFactory failed");
        return 1;
    }
    Camera* camera = cameraFactory->create_camera(0.0f, 50.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    camera->transformations = add_camera_transformation(camera->transformations, &camera->transformationsNum, 0.0f, 0.0f, 0.0f, 0.0f, CAMERA_TRANSFORM_ROTATE);

    const IPuppetFactory *puppetFactory = get_puppetFactory();
    if (puppetFactory == NULL) {
        SDL_Log("get_puppetFactory failed");
        return 1;
    }

    painter->clear_buffer(0x1100);
    painter->draw_buffer();
    uint32_t t = 0;
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = 0;
            }
        }
        float qt = t * 0.002f;
        modify_mesh_transformation(room->transformations, qt, 0.0f, -10.0f, 0.0f, 0);
        modify_mesh_transformation(mug->transformations, qt, 10.0f, -10.0f, 10.0f, 0);
        update_camera(camera);
        modify_camera_transformation(camera->transformations, 0.00f, 0.0f, 1.0f, 0.0f, 0);
        renderer->clean_scene();
        renderer->add_model_to_scene(room, camera, pointLight);
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

    SDL_Quit();
    return 0;
}
