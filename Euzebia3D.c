#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#if defined(EUZEBIA3D_PLATFORM_PICO)
#elif defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#else
#error "Unsupported Euzebia3D platform"
#endif

#include "engineApi.h"

static char t_char[11];

#if EUZEBIA3D_DEBUG_STAGE_ENABLED
volatile uint32_t euzebia_debug_stage = 0;
#define EUZEBIA3D_SET_DEBUG_STAGE(stage)                                       \
  do {                                                                         \
    euzebia_debug_stage = (stage);                                             \
  } while (0)
#else
#define EUZEBIA3D_SET_DEBUG_STAGE(stage) ((void)0)
#endif
volatile uint32_t euzebia_debug_frame = 0;

static e3d_EngineContext engine_ctx;

#define EUZEBIA3D_PLASMA_COLORS_NUM 16

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#define EUZEBIA3D_WINDOWS_TARGET_FPS 24u
#define EUZEBIA3D_PLASMA_SCALE 1
#define EUZEBIA3D_PLASMA_FAC_A 7
#define EUZEBIA3D_PLASMA_FAC_B 7
#define EUZEBIA3D_PLASMA_FAC_C 8
#define EUZEBIA3D_PLASMA_FAC_D 7
#else
#define EUZEBIA3D_PLASMA_SCALE 2
#define EUZEBIA3D_PLASMA_FAC_A 6
#define EUZEBIA3D_PLASMA_FAC_B 6
#define EUZEBIA3D_PLASMA_FAC_C 7
#define EUZEBIA3D_PLASMA_FAC_D 6
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
static int process_window_events(void) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      return 0;
    }
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
      return 0;
    }
  }

  return 1;
}

static void cap_window_frame_rate(uint64_t frame_begin_ticks) {
  uint64_t performance_frequency = SDL_GetPerformanceFrequency();
  if (performance_frequency == 0u || EUZEBIA3D_WINDOWS_TARGET_FPS == 0u) {
    return;
  }

  uint64_t target_frame_ticks =
      performance_frequency / EUZEBIA3D_WINDOWS_TARGET_FPS;
  uint64_t elapsed_ticks = SDL_GetPerformanceCounter() - frame_begin_ticks;
  if (elapsed_ticks >= target_frame_ticks) {
    return;
  }

  uint64_t remaining_ticks = target_frame_ticks - elapsed_ticks;
  uint64_t remaining_ms = (remaining_ticks * 1000u) / performance_frequency;
  if (remaining_ms > 0u) {
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

  e3d_InitEngine(&engine_ctx);

  e3d_Material *mugMat =
      e3d_Material_CreateTexturedMat(&engine_ctx, 0, 0.0f, 0.0f, false);
  e3d_Mesh *mug = e3d_Mesh_CreateMesh(&engine_ctx, mugMat, 1);
  e3d_Mesh_AddTransformation(&engine_ctx, mug, 0, 0.0f, 0.0f, 0.0f,
                             MODEL_TRANSFORM_ROTATE);
  e3d_Mesh_AddTransformation(&engine_ctx, mug, 0, 0.0f, 0.0f, 0.3f,
                             MODEL_TRANSFORM_TRANSLATE);

  e3d_Material *roomMat =
      e3d_Material_CreateTexturedMat(&engine_ctx, 1, 0.0f, 0.0f, false);
  e3d_Mesh *room = e3d_Mesh_CreateMesh(&engine_ctx, roomMat, 2);
  e3d_Mesh_AddTransformation(&engine_ctx, room, 0.2f, 0.0f, 1.0f, 0.0f,
                             MODEL_TRANSFORM_ROTATE);
  e3d_Mesh_AddTransformation(&engine_ctx, room, 0, 2.2f, 2.2f, 2.2f,
                             MODEL_TRANSFORM_SCALE);

  e3d_Light *light = e3d_Light_CreatePointLight(
      &engine_ctx, -10.0f, 3.0f, 15.0f, 15.0f, 0xffff);
  if (light == NULL)
    return 1;
  e3d_Renderer_SetLight(&engine_ctx, light);

  e3d_Camera *camera = e3d_Camera_CreateCamera(
      &engine_ctx, 0.0f, 75.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
      0.0f);
  if (camera == NULL)
    return 1;
  e3d_Renderer_SetCamera(&engine_ctx, camera);

  e3d_Buffer_ClearBuffer(&engine_ctx, 0x0000);
  e3d_Buffer_DrawBuffer(&engine_ctx);

  uint32_t t = 0;

  uint16_t plasmaColors[EUZEBIA3D_PLASMA_COLORS_NUM] = {
      0x1be6, 0x2427, 0x3447, 0x4488, 0x54c8, 0x5d09, 0x6d49, 0x7d8a,
      0x7d8a, 0x6d49, 0x5d09, 0x54c8, 0x4488, 0x3447, 0x2427, 0x1be6,
  };
  e3d_Rectangle plasmaRect = {
      .x = 28,
      .y = 44,
      .height = 181,
      .width = 241,
  };
  e3d_Rectangle bar1 = {
      .x = 0,
      .y = 0,
      .height = 6,
      .width = 320,
  };
  e3d_Point lineStart = {
      .x = 0,
      .y = 0,
  };
  e3d_Point lineEnd = {
      .x = 100,
      .y = 100,
  };
  int running = 1;
  while (running) {
    euzebia_debug_frame = t;
    EUZEBIA3D_SET_DEBUG_STAGE(100);
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    uint64_t frame_begin_ticks = SDL_GetPerformanceCounter();
#endif
#if defined(EUZEBIA3D_DEBUG_MODE)
    e3d_Debug_BeginFrame(&engine_ctx);
#endif

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    running = process_window_events();
#endif
    float qt = t * 0.02f;
    (void)qt;
    e3d_Renderer_CleanScene(&engine_ctx);
    EUZEBIA3D_SET_DEBUG_STAGE(110);
    e3d_Mesh_ModifyTransformation(&engine_ctx, room, qt, 0.0f, -10.0f, 0.0f,
                                  0);
    e3d_Mesh_ModifyTransformation(&engine_ctx, mug, qt, 10.0f, -10.0f, 10.0f,
                                  0);
    EUZEBIA3D_SET_DEBUG_STAGE(120);
    e3d_Renderer_AddModelToScene(&engine_ctx, room);
    e3d_Renderer_AddModelToScene(&engine_ctx, mug);
    EUZEBIA3D_SET_DEBUG_STAGE(130);
    EUZEBIA3D_SET_DEBUG_STAGE(140);
    e3d_Renderer_RenderScene(&engine_ctx);
    EUZEBIA3D_SET_DEBUG_STAGE(145);

#if defined(EUZEBIA3D_DEBUG_MODE)
    EUZEBIA3D_SET_DEBUG_STAGE(150);
    e3d_Debug_ShowInfo(&engine_ctx);
    snprintf(t_char, sizeof(t_char), "%lu", (unsigned long)t);
    e3d_Painter_Print(&engine_ctx, t_char, 0, 220, 1, 0xffff);
    e3d_Debug_BeginDrawBuffer(&engine_ctx);
#endif
    EUZEBIA3D_SET_DEBUG_STAGE(160);
    e3d_Buffer_DrawBuffer(&engine_ctx);
    EUZEBIA3D_SET_DEBUG_STAGE(165);
#if defined(EUZEBIA3D_DEBUG_MODE)
    e3d_Debug_EndDrawBuffer(&engine_ctx);
#endif
    t++;
    EUZEBIA3D_SET_DEBUG_STAGE(170);
    e3d_Buffer_ClearBuffer(&engine_ctx, 0);
#if defined(EUZEBIA3D_DEBUG_MODE)
    e3d_Debug_EndFrame(&engine_ctx);
#endif
    EUZEBIA3D_SET_DEBUG_STAGE(200);

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    cap_window_frame_rate(frame_begin_ticks);
#endif
  }

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  SDL_Quit();
#endif

  return 0;
}
