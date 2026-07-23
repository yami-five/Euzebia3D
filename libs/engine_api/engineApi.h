#ifndef ENGINEAPI_h
#define ENGINEAPI_h

#include <stdbool.h>
#include <stdint.h>

#if defined(EUZEBIA3D_PLATFORM_PICO)
#elif defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#else
#error "Unsupported Euzebia3D platform"
#endif

#include "IAudioPlayer.h"
#include "ICameraFactory.h"
#include "IDebugMode.h"
#include "IDisplay.h"
#include "IHardware.h"
#include "ILightFactory.h"
#include "IMaterialFactory.h"
#include "IMeshFactory.h"
#include "IPainter.h"
#include "IPuppeteer.h"
#include "IRenderer.h"
#include "IStorage.h"

#include "audioPlayer.h"
#include "Camera.h"
#include "cameraFactory.h"
#include "debugMode.h"
#include "lightFactory.h"
#include "Material.h"
#include "materialFactory.h"
#include "Mesh.h"
#include "meshFactory.h"
#include "painter.h"
#include "puppeteer.h"
#include "renderer.h"
#include "storage.h"

#if defined(EUZEBIA3D_PLATFORM_PICO)
#include "display.h"
#include "hardware.h"
#define EUZEBIA3D_SYS_CLOCK_KHZ 300000
// #define EUZEBIA3D_SYS_CLOCK_KHZ 150000
#endif

typedef struct {
  const e3d_IAudioPlayer *audioPlayer;
  const e3d_ICameraFactory *cameraFactory;
  const e3d_IDisplay *display;
  const e3d_IDebugMode *debugMode;
  const e3d_IHardware *hardware;
  const e3d_ILightFactory *lightFactory;
  const e3d_IMaterialFactory *materialFactory;
  const e3d_IMeshFactory *meshFactory;
  const e3d_IPainter *painter;
  const e3d_IPuppeteer *puppeteer;
  const e3d_IRenderer *renderer;
  const e3d_IStorage *storage;
} e3d_EngineContext;

void e3d_InitEngine(e3d_EngineContext *engine_ctx);
// debug
void e3d_Debug_BeginFrame(e3d_EngineContext *engine_ctx);
void e3d_Debug_BeginDrawBuffer(e3d_EngineContext *engine_ctx);
void e3d_Debug_EndDrawBuffer(e3d_EngineContext *engine_ctx);
void e3d_Debug_EndFrame(e3d_EngineContext *engine_ctx);
void e3d_Debug_ResetWindow(e3d_EngineContext *engine_ctx);
void e3d_Debug_ShowInfo(e3d_EngineContext *engine_ctx);
// buffer
void e3d_Buffer_DrawBuffer(e3d_EngineContext *engine_ctx);
void e3d_Buffer_ClearBuffer(e3d_EngineContext *engine_ctx, uint16_t color);
// painter
void e3d_Painter_DrawPixel(e3d_EngineContext *engine_ctx, uint16_t x, uint16_t y,
                           uint16_t color);
void e3d_Painter_DrawImage(e3d_EngineContext *engine_ctx, uint8_t image_index);
void e3d_Painter_DrawSprite(e3d_EngineContext *engine_ctx, const e3d_Sprite *e3d_Sprite,
                            int16_t pos_x, int16_t pos_y, int32_t angle,
                            uint8_t scale);
void e3d_Painter_DrawBackground(e3d_EngineContext *engine_ctx, e3d_Image *e3d_Image);
void e3d_Painter_Print(e3d_EngineContext *engine_ctx, const char *text, int16_t x,
                       int16_t y, uint8_t fontIndex, uint16_t color);
void e3d_Painter_FadeFullscreen(e3d_EngineContext *engine_ctx, uint8_t mode,
                                uint32_t startFrame, uint32_t currentFrame);
void e3d_Painter_DrawScroller(e3d_EngineContext *engine_ctx,
                              const e3d_Scroller *e3d_Scroller, uint16_t x, uint16_t y,
                              uint32_t startFrame, uint32_t currentFrame);
void e3d_Painter_DrawPlasma(e3d_EngineContext *engine_ctx, uint16_t *colors,
                            uint16_t colorsNum, uint32_t t, uint8_t scale,
                            int8_t facA, int8_t facB, int8_t facC, int8_t facD,
                            e3d_Rectangle *e3d_Rectangle);
void e3d_Painter_DrawRectangle(e3d_EngineContext *engine_ctx, e3d_Rectangle *rect,
                               uint16_t color);
void e3d_Painter_DrawLine(e3d_EngineContext *engine_ctx, e3d_Point *start, e3d_Point *end,
                          uint16_t color);
void e3d_Painter_DrawGradient(e3d_EngineContext *engine_ctx, uint16_t colorA,
                              uint16_t colorB, e3d_Rectangle *e3d_Rectangle,
                              uint8_t direction);
// puppetteer
e3d_Puppet *e3d_Puppetteer_CreatePuppet(e3d_EngineContext *engine_ctx,
                                    uint8_t puppetIndex);
void e3d_Puppetteer_Perform(e3d_EngineContext *engine_ctx, e3d_Puppet *e3d_Puppet,
                            uint32_t t);
// e3d_Camera
e3d_Camera *e3d_Camera_CreateCamera(e3d_EngineContext *engine_ctx, float camX,
                                float camY, float camZ, float targetX,
                                float targetY, float targetZ, float upX,
                                float upY, float upZ);
// e3d_Light
e3d_Light *e3d_Light_CreatePointLight(e3d_EngineContext *engine_ctx, float x, float y,
                                  float z, float intensity, uint16_t color);
e3d_Light *e3d_Light_CreateDirectionalLight(e3d_EngineContext *engine_ctx, float x,
                                        float y, float z, float intensity,
                                        uint16_t color);
// e3d_Mesh
e3d_Mesh *e3d_Mesh_CreateMesh(e3d_EngineContext *engine_ctx, e3d_Material *mat,
                          uint8_t meshIndex);
e3d_TransformInfo *e3d_Mesh_AddTransformation(
    e3d_EngineContext *engine_ctx, e3d_TransformInfo *currentTransformations,
    uint32_t *currentTransformationsNum, float w, float x, float y, float z,
    e3d_ModelTransformType transformationType);
void e3d_Mesh_ModifyTransformation(
    e3d_EngineContext *engine_ctx, e3d_TransformInfo *currentTransformations,
    float w, float x, float y, float z, uint32_t transformationIndex);
// e3d_Material
e3d_Material *e3d_Material_CreateDiffuseMat(e3d_EngineContext *engine_ctx,
                                        uint16_t color, float roughness,
                                        float metallic);
e3d_Material *e3d_Material_CreateTexturedMat(e3d_EngineContext *engine_ctx,
                                         uint8_t imageIndex, float roughness,
                                         float metallic, bool transparent);
// renderer
void e3d_Renderer_SetRendererScale(e3d_EngineContext *engine_ctx, uint8_t scale);
void e3d_Renderer_AddModelToScene(e3d_EngineContext *engine_ctx, e3d_Mesh *e3d_Mesh);
void e3d_Renderer_AddPointToScene(e3d_EngineContext *engine_ctx, e3d_Point3D *e3d_Point);
void e3d_Renderer_AddLineToScene(e3d_EngineContext *engine_ctx, e3d_Line3D *line);
void e3d_Renderer_CleanScene(e3d_EngineContext *engine_ctx);
void e3d_Renderer_RenderScene(e3d_EngineContext *engine_ctx);
void e3d_Renderer_SetCamera(e3d_EngineContext *engine_ctx, e3d_Camera *e3d_Camera);
void e3d_Renderer_SetLight(e3d_EngineContext *engine_ctx, e3d_Light *e3d_Light);
//audioPlayer
void e3d_Audio_PlayWavFile(e3d_EngineContext *engine_ctx, char *file_name);

#endif
