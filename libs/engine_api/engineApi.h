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

/// Initializes all engine subsystems and stores their interfaces in the context.
void e3d_InitEngine(e3d_EngineContext *engine_ctx);
// debug
/// Starts collecting debug information for a new frame.
void e3d_Debug_BeginFrame(e3d_EngineContext *engine_ctx);
/// Starts measuring the operation that draws the frame buffer.
void e3d_Debug_BeginDrawBuffer(e3d_EngineContext *engine_ctx);
/// Finishes measuring the operation that draws the frame buffer.
void e3d_Debug_EndDrawBuffer(e3d_EngineContext *engine_ctx);
/// Finishes collecting debug information for the current frame.
void e3d_Debug_EndFrame(e3d_EngineContext *engine_ctx);
/// Resets the debug output window.
void e3d_Debug_ResetWindow(e3d_EngineContext *engine_ctx);
/// Displays the currently collected debug information.
void e3d_Debug_ShowInfo(e3d_EngineContext *engine_ctx);
// buffer
/// Presents the painter's frame buffer on the display.
void e3d_Buffer_DrawBuffer(e3d_EngineContext *engine_ctx);
/// Fills the entire frame buffer with the specified color.
void e3d_Buffer_ClearBuffer(e3d_EngineContext *engine_ctx, uint16_t color);
// painter
/// Draws a pixel at the specified screen coordinates.
void e3d_Painter_DrawPixel(e3d_EngineContext *engine_ctx, uint16_t x, uint16_t y,
                           uint16_t color);
/// Draws an image resource selected by its index.
void e3d_Painter_DrawImage(e3d_EngineContext *engine_ctx, uint8_t image_index);
/// Draws a sprite at the specified position, rotation and scale.
void e3d_Painter_DrawSprite(e3d_EngineContext *engine_ctx, const e3d_Sprite *e3d_Sprite,
                            int16_t pos_x, int16_t pos_y, int32_t angle,
                            uint8_t scale);
/// Draws an image as the screen background.
void e3d_Painter_DrawBackground(e3d_EngineContext *engine_ctx, e3d_Image *e3d_Image);
/// Draws text at the specified screen position using the selected font and color.
void e3d_Painter_Print(e3d_EngineContext *engine_ctx, const char *text, int16_t x,
                       int16_t y, uint8_t fontIndex, uint16_t color);
/// Applies a full-screen fade effect based on the selected mode and frame timing.
void e3d_Painter_FadeFullscreen(e3d_EngineContext *engine_ctx, uint8_t mode,
                                uint32_t startFrame, uint32_t currentFrame);
/// Draws an animated text scroller using the supplied frame timing.
void e3d_Painter_DrawScroller(e3d_EngineContext *engine_ctx,
                              const e3d_Scroller *e3d_Scroller, uint16_t x, uint16_t y,
                              uint32_t startFrame, uint32_t currentFrame);
/// Draws an animated plasma effect inside the specified rectangle.
void e3d_Painter_DrawPlasma(e3d_EngineContext *engine_ctx, uint16_t *colors,
                            uint16_t colorsNum, uint32_t t, uint8_t scale,
                            int8_t facA, int8_t facB, int8_t facC, int8_t facD,
                            e3d_Rectangle *e3d_Rectangle);
/// Fills the specified rectangle with a solid color.
void e3d_Painter_DrawRectangle(e3d_EngineContext *engine_ctx, e3d_Rectangle *rect,
                               uint16_t color);
/// Draws a two-dimensional line between the supplied points.
void e3d_Painter_DrawLine(e3d_EngineContext *engine_ctx, e3d_Point *start, e3d_Point *end,
                          uint16_t color);
/// Fills a rectangle with a directional gradient between two colors.
void e3d_Painter_DrawGradient(e3d_EngineContext *engine_ctx, uint16_t colorA,
                              uint16_t colorB, e3d_Rectangle *e3d_Rectangle,
                              uint8_t direction);
// puppetteer
/// Creates a puppet from the resource selected by its index.
e3d_Puppet *e3d_Puppetteer_CreatePuppet(e3d_EngineContext *engine_ctx,
                                    uint8_t puppetIndex);
/// Updates and performs a puppet animation for the specified time value.
void e3d_Puppetteer_Perform(e3d_EngineContext *engine_ctx, e3d_Puppet *e3d_Puppet,
                            uint32_t t);
// e3d_Camera
/// Creates a camera from its position, target and up vectors.
e3d_Camera *e3d_Camera_CreateCamera(e3d_EngineContext *engine_ctx, float camX,
                                float camY, float camZ, float targetX,
                                float targetY, float targetZ, float upX,
                                float upY, float upZ);
// e3d_Light
/// Creates a point light at the specified position.
e3d_Light *e3d_Light_CreatePointLight(e3d_EngineContext *engine_ctx, float x, float y,
                                  float z, float intensity, uint16_t color);
/// Creates a directional light using the specified direction.
e3d_Light *e3d_Light_CreateDirectionalLight(e3d_EngineContext *engine_ctx, float x,
                                        float y, float z, float intensity,
                                        uint16_t color);
// e3d_Mesh
/// Creates a mesh from the indexed mesh resource and assigns its material.
e3d_Mesh *e3d_Mesh_CreateMesh(e3d_EngineContext *engine_ctx, e3d_Material *mat,
                          uint8_t meshIndex);
/// Appends a transformation to a mesh transformation list and updates its count.
e3d_TransformInfo *e3d_Mesh_AddTransformation(
    e3d_EngineContext *engine_ctx, e3d_TransformInfo *currentTransformations,
    uint32_t *currentTransformationsNum, float w, float x, float y, float z,
    e3d_ModelTransformType transformationType);
/// Replaces the values of an existing mesh transformation at the given index.
void e3d_Mesh_ModifyTransformation(
    e3d_EngineContext *engine_ctx, e3d_TransformInfo *currentTransformations,
    float w, float x, float y, float z, uint32_t transformationIndex);
// e3d_Material
/// Creates a diffuse material with the specified color and surface properties.
e3d_Material *e3d_Material_CreateDiffuseMat(e3d_EngineContext *engine_ctx,
                                        uint16_t color, float roughness,
                                        float metallic);
/// Creates a textured material from an indexed image resource.
e3d_Material *e3d_Material_CreateTexturedMat(e3d_EngineContext *engine_ctx,
                                         uint8_t imageIndex, float roughness,
                                         float metallic, bool transparent);
// renderer
/// Sets the renderer resolution scale.
void e3d_Renderer_SetRendererScale(e3d_EngineContext *engine_ctx, uint8_t scale);
/// Transforms a mesh and adds its visible triangles to the current scene.
void e3d_Renderer_AddModelToScene(e3d_EngineContext *engine_ctx, e3d_Mesh *e3d_Mesh);
/// Transforms a 3D point and adds it to the current scene when visible.
void e3d_Renderer_AddPointToScene(e3d_EngineContext *engine_ctx, e3d_Point3D *e3d_Point);
/// Transforms and clips a 3D line, then adds it to the current scene when visible.
void e3d_Renderer_AddLineToScene(e3d_EngineContext *engine_ctx, e3d_Line3D *line);
/// Removes all objects from the current scene and resets its counters.
void e3d_Renderer_CleanScene(e3d_EngineContext *engine_ctx);
/// Sorts and renders every object currently stored in the scene.
void e3d_Renderer_RenderScene(e3d_EngineContext *engine_ctx);
/// Selects the camera used for subsequent scene transformations and rendering.
void e3d_Renderer_SetCamera(e3d_EngineContext *engine_ctx, e3d_Camera *e3d_Camera);
/// Selects the light used for shading subsequently added models.
void e3d_Renderer_SetLight(e3d_EngineContext *engine_ctx, e3d_Light *e3d_Light);
//audioPlayer
/// Starts playback of a WAV file from storage.
void e3d_Audio_PlayWavFile(e3d_EngineContext *engine_ctx, char *file_name);

#endif
