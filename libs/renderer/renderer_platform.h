#ifndef RENDERER_PLATFORM_H
#define RENDERER_PLATFORM_H

#include "IRenderer.h"
#include <stdbool.h>

typedef enum {
  TEXTURE_SOURCE_UNKNOWN = 0,
  TEXTURE_SOURCE_FLASH = 1,
  TEXTURE_SOURCE_PSRAM = 2
} e3d_TextureSource;

void renderer_platform_init(const e3d_IHardware *hardware);
e3d_TextureSource
renderer_platform_detect_texture_source(const uint16_t *texture);
void renderer_platform_texture_span(
    uint16_t *dst, uint16_t length, const uint16_t *texture,
    int32_t texture_width, int32_t texture_height,
    int32_t texture_width_shift, int32_t texture_height_shift,
    bool transparent, int32_t u, int32_t du_dx, int32_t v, int32_t dv_dx,
    int32_t z, int32_t dz_dx);
void renderer_platform_shade_span(uint16_t *dst, uint16_t length,
                                  const e3d_Light *light, int32_t light_value,
                                  int32_t light_delta);

#endif
