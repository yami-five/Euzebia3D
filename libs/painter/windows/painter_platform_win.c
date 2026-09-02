#include "painter_platform.h"
#include "painter.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

#ifndef EUZEBIA3D_WINDOWS_WINDOW_WIDTH
#define EUZEBIA3D_WINDOWS_WINDOW_WIDTH (DISPLAY_WIDTH * 3)
#endif

#ifndef EUZEBIA3D_WINDOWS_WINDOW_HEIGHT
#define EUZEBIA3D_WINDOWS_WINDOW_HEIGHT (DISPLAY_HEIGHT * 3)
#endif

#ifndef EUZEBIA3D_WINDOWS_FULLSCREEN
#define EUZEBIA3D_WINDOWS_FULLSCREEN 0
#endif

static uint16_t mirrored_buffer[BUFFER_SIZE_HALF];
static SDL_Window *sdl_window = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture *sdl_texture = NULL;
static uint8_t sdl_video_initialized_here = 0;
static uint8_t sdl_cleanup_registered = 0;

static void set_debug_stage(volatile uint32_t *debug_stage, uint32_t stage) {
  if (debug_stage != NULL)
    *debug_stage = stage;
}

static void destroy_sdl_backend(void) {
  if (sdl_texture != NULL) {
    SDL_DestroyTexture(sdl_texture);
    sdl_texture = NULL;
  }
  if (sdl_renderer != NULL) {
    SDL_DestroyRenderer(sdl_renderer);
    sdl_renderer = NULL;
  }
  if (sdl_window != NULL) {
    SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
  }
  if (sdl_video_initialized_here) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    sdl_video_initialized_here = 0;
  }
}

static bool ensure_sdl_backend(void) {
  if (sdl_texture != NULL && sdl_renderer != NULL && sdl_window != NULL)
    return true;

  if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
      SDL_Log("Painter SDL init failed: %s", SDL_GetError());
      return false;
    }
    sdl_video_initialized_here = 1;
  }

#if EUZEBIA3D_WINDOWS_FULLSCREEN
  const SDL_WindowFlags window_flags = SDL_WINDOW_FULLSCREEN;
#else
  const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
#endif
  sdl_window = SDL_CreateWindow("Euzebia3D", EUZEBIA3D_WINDOWS_WINDOW_WIDTH,
                                EUZEBIA3D_WINDOWS_WINDOW_HEIGHT, window_flags);
  if (sdl_window == NULL) {
    SDL_Log("Painter SDL window creation failed: %s", SDL_GetError());
    destroy_sdl_backend();
    return false;
  }

  sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
  if (sdl_renderer == NULL) {
    SDL_Log("Painter SDL renderer creation failed: %s", SDL_GetError());
    destroy_sdl_backend();
    return false;
  }
  SDL_SetDefaultTextureScaleMode(sdl_renderer, SDL_SCALEMODE_LINEAR);

  sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH,
                                  DISPLAY_HEIGHT);
  if (sdl_texture == NULL) {
    SDL_Log("Painter SDL texture creation failed: %s", SDL_GetError());
    destroy_sdl_backend();
    return false;
  }
  SDL_SetTextureScaleMode(sdl_texture, SDL_SCALEMODE_LINEAR);

  SDL_SetRenderLogicalPresentation(sdl_renderer, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  if (!sdl_cleanup_registered) {
    atexit(destroy_sdl_backend);
    sdl_cleanup_registered = 1;
  }
  return true;
}

void painter_platform_init(const e3d_IDisplay *display) {
  (void)display;
  (void)ensure_sdl_backend();
}

bool painter_platform_draw_buffer(const uint16_t *buffer,
                                  volatile uint32_t *debug_stage,
                                  volatile uint32_t *debug_line) {
  if (!ensure_sdl_backend())
    return false;

  for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
    if (debug_line != NULL)
      *debug_line = y;

    uint32_t dst_row_start = (uint32_t)y * DISPLAY_WIDTH;
    uint32_t src_row_start = (uint32_t)(DISPLAY_HEIGHT - 1u - y) * DISPLAY_WIDTH;
    for (uint16_t x = 0; x < DISPLAY_WIDTH; x++)
      mirrored_buffer[dst_row_start + x] = buffer[src_row_start + x];
  }

  set_debug_stage(debug_stage, 110);
  SDL_UpdateTexture(sdl_texture, NULL, mirrored_buffer,
                    DISPLAY_WIDTH * (int32_t)sizeof(uint16_t));
  SDL_RenderClear(sdl_renderer);
  SDL_RenderTexture(sdl_renderer, sdl_texture, NULL, NULL);
  set_debug_stage(debug_stage, 120);
  SDL_RenderPresent(sdl_renderer);
  return true;
}
