#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
/* SDL_main.h is part of the public API, but the engine library must not
 * provide the application's main/WinMain implementation. */
#define SDL_MAIN_NOIMPL 1
#endif

#include "engineApi.h"
#include <string.h>
#if defined(EUZEBIA3D_DEBUG_MODE)
static const e3d_IDebugMode *debugMode;
#endif

void e3d_InitEngine(e3d_EngineContext *engine_ctx) {

#if defined(EUZEBIA3D_PLATFORM_PICO)
  set_sys_clock_khz(EUZEBIA3D_SYS_CLOCK_KHZ, true);

  engine_ctx->hardware = get_hardware();
  engine_ctx->hardware->init_hardware();

  engine_ctx->display = get_display();
  engine_ctx->display->init_display(engine_ctx->hardware);
#endif

  engine_ctx->storage = get_storage();

  engine_ctx->painter = get_painter();
  engine_ctx->painter->init_painter(engine_ctx->display, engine_ctx->hardware,
                                    engine_ctx->storage);

  // #if defined(EUZEBIA3D_DEBUG_MODE)
  engine_ctx->debugMode = get_debugMode();
  engine_ctx->debugMode->init_debug_mode(engine_ctx->hardware,
                                         engine_ctx->painter);
  // #endif

  engine_ctx->puppeteer = get_puppeteer();
  engine_ctx->puppeteer->init_puppeteer(engine_ctx->storage,
                                        engine_ctx->painter);

  engine_ctx->renderer = get_renderer();
  engine_ctx->renderer->init_renderer(engine_ctx->hardware,
                                      engine_ctx->painter);
  engine_ctx->renderer->set_scale(1);

  engine_ctx->materialFactory = get_materialFactory();
  engine_ctx->materialFactory->init_material_factory(engine_ctx->storage);

  engine_ctx->meshFactory = get_meshFactory();
  engine_ctx->meshFactory->init_mesh_factory(engine_ctx->storage);

  engine_ctx->lightFactory = get_lightFactory();

  engine_ctx->cameraFactory = get_cameraFactory();
}
// debug
void e3d_Debug_BeginFrame(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->begin_frame();
}

void e3d_Debug_BeginDrawBuffer(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->begin_draw_buffer();
}

void e3d_Debug_EndDrawBuffer(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->end_draw_buffer();
}

void e3d_Debug_EndFrame(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->end_frame();
}

void e3d_Debug_ResetWindow(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->reset_window();
}

void e3d_Debug_ShowInfo(e3d_EngineContext *engine_ctx) {
  engine_ctx->debugMode->show_info();
}
// buffer
void e3d_Buffer_DrawBuffer(e3d_EngineContext *engine_ctx) {
  engine_ctx->painter->draw_buffer();
}

void e3d_Buffer_ClearBuffer(e3d_EngineContext *engine_ctx, uint16_t color) {
  engine_ctx->painter->clear_buffer(color);
}

// painter
void e3d_Painter_DrawPixel(e3d_EngineContext *engine_ctx, uint16_t x,
                           uint16_t y, uint16_t color) {
  engine_ctx->painter->draw_pixel(x, y, color);
}

void e3d_Painter_DrawImage(e3d_EngineContext *engine_ctx, uint8_t image_index) {
  engine_ctx->painter->draw_image(image_index);
}

void e3d_Painter_DrawSprite(e3d_EngineContext *engine_ctx,
                            const e3d_Sprite *e3d_Sprite, int16_t pos_x,
                            int16_t pos_y, int32_t angle, uint8_t scale) {
  engine_ctx->painter->draw_sprite(e3d_Sprite, pos_x, pos_y, angle, scale);
}

void e3d_Painter_DrawBackground(e3d_EngineContext *engine_ctx,
                                e3d_Image *e3d_Image) {
  engine_ctx->painter->draw_background(e3d_Image);
}

void e3d_Painter_Print(e3d_EngineContext *engine_ctx, const char *text,
                       int16_t x, int16_t y, uint8_t fontIndex,
                       uint16_t color) {
  engine_ctx->painter->print(text, x, y, fontIndex, color);
}

void e3d_Painter_DrawScroller(e3d_EngineContext *engine_ctx,
                              const e3d_Scroller *e3d_Scroller, uint16_t x,
                              uint16_t y, uint32_t startFrame,
                              uint32_t currentFrame) {
  engine_ctx->painter->draw_scroller(e3d_Scroller, x, y, startFrame,
                                     currentFrame);
}

void e3d_Painter_DrawPlasma(e3d_EngineContext *engine_ctx, uint16_t *colors,
                            uint16_t colorsNum, uint32_t t, uint8_t scale,
                            int8_t facA, int8_t facB, int8_t facC, int8_t facD,
                            e3d_Rectangle *e3d_Rectangle) {
  engine_ctx->painter->draw_plasma(colors, colorsNum, t, scale, facA, facB,
                                   facC, facD, e3d_Rectangle);
}

void e3d_Painter_DrawRectangle(e3d_EngineContext *engine_ctx,
                               e3d_Rectangle *rect, uint16_t color) {
  engine_ctx->painter->draw_rectangle(rect, color);
}

void e3d_Painter_DrawLine(e3d_EngineContext *engine_ctx, e3d_Point *start,
                          e3d_Point *end, uint16_t color) {
  engine_ctx->painter->draw_line(start, end, color);
}

void e3d_Painter_DrawGradient(e3d_EngineContext *engine_ctx, uint16_t colorA,
                              uint16_t colorB, e3d_Rectangle *e3d_Rectangle,
                              uint8_t direction) {
  engine_ctx->painter->draw_gradient(colorA, colorB, e3d_Rectangle, direction);
}

// puppetteer
e3d_Puppet *e3d_Puppetteer_CreatePuppet(e3d_EngineContext *engine_ctx,
                                        uint8_t puppetIndex) {
  return engine_ctx->puppeteer->create_puppet(puppetIndex);
}

void e3d_Puppetteer_Perform(e3d_EngineContext *engine_ctx,
                            e3d_Puppet *e3d_Puppet, uint32_t t) {
  engine_ctx->puppeteer->perform(e3d_Puppet, t);
}

// e3d_Camera
e3d_Camera *e3d_Camera_CreateCamera(e3d_EngineContext *engine_ctx, float camX,
                                    float camY, float camZ, float targetX,
                                    float targetY, float targetZ, float upX,
                                    float upY, float upZ) {
  return engine_ctx->cameraFactory->create_camera(
      camX, camY, camZ, targetX, targetY, targetZ, upX, upY, upZ);
}

void e3d_Camera_SetPos(e3d_EngineContext *engine_ctx, e3d_Camera *camera,
                       float x, float y, float z) {
  (void)engine_ctx;
  if (camera == NULL)
    return;
  set_camera_pos(camera, x, y, z);
}

void e3d_Camera_SetTargetPos(e3d_EngineContext *engine_ctx, e3d_Camera *camera,
                             float x, float y, float z) {
  (void)engine_ctx;
  if (camera == NULL)
    return;
  set_camera_target_pos(camera, x, y, z);
}

void e3d_Camera_UpdateCamera(e3d_EngineContext *engine_ctx,
                             e3d_Camera *camera) {
  (void)engine_ctx;
  update_camera(camera);
}

void e3d_Camera_DeleteCamera(e3d_EngineContext *engine_ctx,
                             e3d_Camera **camera) {
  if (camera == NULL || *camera == NULL)
    return;

  if (engine_ctx != NULL && engine_ctx->renderer != NULL)
    engine_ctx->renderer->unset_camera(*camera);

  free_camera(*camera);
  *camera = NULL;
}

// e3d_Light
e3d_Light *e3d_Light_CreatePointLight(e3d_EngineContext *engine_ctx, float x,
                                      float y, float z, float intensity,
                                      uint16_t color) {
  return engine_ctx->lightFactory->create_point_light(x, y, z, intensity,
                                                      color);
}

e3d_Light *e3d_Light_CreateDirectionalLight(e3d_EngineContext *engine_ctx,
                                            float x, float y, float z,
                                            float intensity, uint16_t color) {
  return engine_ctx->lightFactory->create_directional_light(x, y, z, intensity,
                                                            color);
}
void e3d_Light_DeleteLight(e3d_EngineContext *engine_ctx, e3d_Light **light) {
  if (light == NULL || *light == NULL)
    return;

  if (engine_ctx != NULL && engine_ctx->renderer != NULL)
    engine_ctx->renderer->unset_light(*light);

  free(*light);
  *light = NULL;
}

// e3d_Mesh
e3d_Mesh *e3d_Mesh_CreateMesh(e3d_EngineContext *engine_ctx,
                              const e3d_Material *mat, uint8_t meshIndex) {
  return engine_ctx->meshFactory->create_mesh(mat, meshIndex);
}

void e3d_Mesh_DeleteMesh(e3d_EngineContext *engine_ctx, e3d_Mesh **mesh) {
  (void)engine_ctx;
  if (mesh == NULL || *mesh == NULL)
    return;

  free_model(*mesh);
  *mesh = NULL;
}

e3d_TransformInfo *
e3d_Mesh_AddTransformation(e3d_EngineContext *engine_ctx, e3d_Mesh *mesh,
                           float w, float x, float y, float z,
                           e3d_ModelTransformType transformationType) {
  (void)engine_ctx;
  if (mesh == NULL)
    return NULL;

  mesh->transformations =
      add_transformation(mesh->transformations, &mesh->transformationsNum, w, x,
                         y, z, transformationType);
  return mesh->transformations;
}

void e3d_Mesh_ModifyTransformation(e3d_EngineContext *engine_ctx,
                                   e3d_Mesh *mesh, float w, float x, float y,
                                   float z, uint32_t transformationIndex) {
  (void)engine_ctx;
  if (mesh == NULL || transformationIndex >= mesh->transformationsNum)
    return;

  modify_mesh_transformation(mesh->transformations, w, x, y, z,
                             transformationIndex);
}

// e3d_Material
e3d_Material *e3d_Material_CreateDiffuseMat(e3d_EngineContext *engine_ctx,
                                            uint16_t color, float roughness,
                                            float metallic) {
  return engine_ctx->materialFactory->create_diffuse_mat(color, roughness,
                                                         metallic);
}

e3d_Material *e3d_Material_CreateTexturedMat(e3d_EngineContext *engine_ctx,
                                             uint8_t imageIndex,
                                             float roughness, float metallic,
                                             bool transparent) {
  return engine_ctx->materialFactory->create_textured_mat(
      imageIndex, roughness, metallic, transparent);
}

void e3d_Material_DeleteMat(e3d_EngineContext *engine_ctx, e3d_Material **mat) {
  if (mat == NULL || *mat == NULL)
    return;

  if (engine_ctx != NULL && engine_ctx->renderer != NULL)
    engine_ctx->renderer->remove_material_from_scene(*mat);

  free_material(*mat);
  *mat = NULL;
}

// renderer
void e3d_Renderer_SetRendererScale(e3d_EngineContext *engine_ctx,
                                   uint8_t scale) {
  engine_ctx->renderer->set_scale(scale);
}

void e3d_Renderer_AddModelToScene(e3d_EngineContext *engine_ctx,
                                  e3d_Mesh *e3d_Mesh) {
  engine_ctx->renderer->add_model_to_scene(e3d_Mesh);
}

void e3d_Renderer_AddPointToScene(e3d_EngineContext *engine_ctx,
                                  e3d_Point3D *e3d_Point) {
  engine_ctx->renderer->add_point_to_scene(e3d_Point);
}

void e3d_Renderer_AddLineToScene(e3d_EngineContext *engine_ctx,
                                 e3d_Line3D *line) {
  engine_ctx->renderer->add_line_to_scene(line);
}

void e3d_Renderer_CleanScene(e3d_EngineContext *engine_ctx) {
  engine_ctx->renderer->clean_scene();
}

void e3d_Renderer_RenderScene(e3d_EngineContext *engine_ctx) {
  engine_ctx->renderer->render_scene();
}

void e3d_Renderer_SetCamera(e3d_EngineContext *engine_ctx,
                            e3d_Camera *e3d_Camera) {
  engine_ctx->renderer->set_camera(e3d_Camera);
}

void e3d_Renderer_SetLight(e3d_EngineContext *engine_ctx,
                           e3d_Light *e3d_Light) {
  engine_ctx->renderer->set_light(e3d_Light);
}
// audioPlayer
void e3d_Audio_PlayWavFile(e3d_EngineContext *engine_ctx, char *file_name) {
  engine_ctx->audioPlayer->play_wave_file(file_name);
}
