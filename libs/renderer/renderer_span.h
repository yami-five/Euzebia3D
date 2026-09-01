#ifndef RENDERER_SPAN_H
#define RENDERER_SPAN_H

#include "renderer.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int32_t intensity;
  uint32_t r_light_scale;
  uint32_t g_light_scale;
  uint32_t b_light_scale;
} e3d_RendererShadingContext;

static inline int32_t renderer_span_clamp_i64_to_i32(int64_t value) {
  if (value > INT32_MAX)
    return INT32_MAX;
  if (value < INT32_MIN)
    return INT32_MIN;
  return (int32_t)value;
}

static inline int32_t renderer_span_restore_perspective_uv(int32_t uv_over_z,
                                                           int32_t z) {
#if EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED
  int64_t value = (int64_t)uv_over_z * (int64_t)z;
  value >>= (SHIFT_FACTOR + UV_PERSPECTIVE_SHIFT);
  return renderer_span_clamp_i64_to_i32(value);
#else
  (void)z;
  return uv_over_z >> UV_PERSPECTIVE_SHIFT;
#endif
}

static inline e3d_RendererShadingContext
renderer_span_make_shading_context(const e3d_Light *light) {
  int32_t intensity = light->intensity;
  if (intensity < 0)
    intensity = 0;
  const int32_t intensity_max = SCALE_FACTOR * 6;
  if (intensity > intensity_max)
    intensity = intensity_max;

  uint32_t r_light = (light->color >> 11) & 0x1f;
  uint32_t g_light = (light->color >> 5) & 0x3f;
  uint32_t b_light = light->color & 0x1f;

  e3d_RendererShadingContext context = {
      .intensity = intensity,
      .r_light_scale = r_light * 33u,
      .g_light_scale = g_light * 16u,
      .b_light_scale = b_light * 33u,
  };
  return context;
}

static inline uint16_t
renderer_span_shade_color(uint16_t color,
                          const e3d_RendererShadingContext *context,
                          int32_t light_distance) {
  if (color == TEXTURE_TRANSPARENT_COLOR)
    return color;

  const int32_t ambient_min = SCALE_FACTOR >> 5;
  if (light_distance < ambient_min)
    light_distance = ambient_min;
  if (light_distance > SCALE_FACTOR)
    light_distance = SCALE_FACTOR;

  int32_t light_factor = (int32_t)(((int64_t)light_distance *
                                    context->intensity) >>
                                   SHIFT_FACTOR);
  if (light_factor < 0)
    light_factor = 0;
  const int32_t max_light_factor = SCALE_FACTOR * 4;
  if (light_factor > max_light_factor)
    light_factor = max_light_factor;

  uint32_t r_mesh = (color >> 11) & 0x1f;
  uint32_t g_mesh = (color >> 5) & 0x3f;
  uint32_t b_mesh = color & 0x1f;

  uint32_t r_tmp =
      (uint32_t)(((uint64_t)(r_mesh * context->r_light_scale) *
                  (uint32_t)light_factor) >>
                 (SHIFT_FACTOR * 2));
  uint32_t g_tmp =
      (uint32_t)(((uint64_t)(g_mesh * context->g_light_scale) *
                  (uint32_t)light_factor) >>
                 (SHIFT_FACTOR * 2));
  uint32_t b_tmp =
      (uint32_t)(((uint64_t)(b_mesh * context->b_light_scale) *
                  (uint32_t)light_factor) >>
                 (SHIFT_FACTOR * 2));

  if (r_tmp > 31)
    r_tmp = 31;
  if (g_tmp > 63)
    g_tmp = 63;
  if (b_tmp > 31)
    b_tmp = 31;

  return (uint16_t)(((uint16_t)r_tmp << 11) | ((uint16_t)g_tmp << 5) |
                    (uint16_t)b_tmp);
}

static inline void renderer_span_add_opaque_texel(uint16_t color, uint32_t *r,
                                                   uint32_t *g, uint32_t *b,
                                                   uint32_t *count) {
  if (color == TEXTURE_TRANSPARENT_COLOR)
    return;

  *r += (color >> 11) & 0x1f;
  *g += (color >> 5) & 0x3f;
  *b += color & 0x1f;
  *count += 1u;
}

static inline uint16_t renderer_span_average_opaque_rgb565_pair(uint16_t a,
                                                                 uint16_t b) {
  uint32_t rb = ((uint32_t)(a & 0xf81f) + (uint32_t)(b & 0xf81f)) >> 1;
  uint32_t g = ((uint32_t)(a & 0x07e0) + (uint32_t)(b & 0x07e0)) >> 1;
  return (uint16_t)((rb & 0xf81f) | (g & 0x07e0));
}

static inline uint16_t renderer_span_average_opaque_rgb565_2x2(
    uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11) {
  uint16_t top = renderer_span_average_opaque_rgb565_pair(c00, c10);
  uint16_t bottom = renderer_span_average_opaque_rgb565_pair(c01, c11);
  return renderer_span_average_opaque_rgb565_pair(top, bottom);
}

static inline uint16_t renderer_span_sample_texture_2x2(
    const uint16_t *texture, int32_t row0, int32_t row1, int32_t x0,
    int32_t x1, bool transparent) {
  uint16_t c00 = texture[row0 + x0];
#if !EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED
  (void)row1;
  (void)x1;
  (void)transparent;
  return c00;
#else
  uint16_t c10 = texture[row0 + x1];
  uint16_t c01 = texture[row1 + x0];
  uint16_t c11 = texture[row1 + x1];

  if (!transparent)
    return renderer_span_average_opaque_rgb565_2x2(c00, c10, c01, c11);

  if (c00 == TEXTURE_TRANSPARENT_COLOR)
    return TEXTURE_TRANSPARENT_COLOR;

  if (c10 == TEXTURE_TRANSPARENT_COLOR || c01 == TEXTURE_TRANSPARENT_COLOR ||
      c11 == TEXTURE_TRANSPARENT_COLOR) {
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t count = 0;
    renderer_span_add_opaque_texel(c00, &r, &g, &b, &count);
    renderer_span_add_opaque_texel(c10, &r, &g, &b, &count);
    renderer_span_add_opaque_texel(c01, &r, &g, &b, &count);
    renderer_span_add_opaque_texel(c11, &r, &g, &b, &count);
    if (count == 0)
      return TEXTURE_TRANSPARENT_COLOR;
    return (uint16_t)(((r / count) << 11) | ((g / count) << 5) | (b / count));
  }

  return renderer_span_average_opaque_rgb565_2x2(c00, c10, c01, c11);
#endif
}

static inline void renderer_span_clamp_uv_fixed(int32_t *uv_x,
                                                 int32_t *uv_y) {
  if (*uv_x < 0)
    *uv_x = 0;
  if (*uv_y < 0)
    *uv_y = 0;
  if (*uv_x > SCALE_FACTOR)
    *uv_x = SCALE_FACTOR;
  if (*uv_y > SCALE_FACTOR)
    *uv_y = SCALE_FACTOR;
}

static inline void renderer_span_clamp_texel_coords(
    int32_t texture_width, int32_t texture_height, int32_t *tex_x,
    int32_t *tex_y) {
  int32_t min_x = texture_width > 2 ? 1 : 0;
  int32_t min_y = texture_height > 2 ? 1 : 0;
  int32_t max_x = texture_width > 2 ? texture_width - 2 : texture_width - 1;
  int32_t max_y = texture_height > 2 ? texture_height - 2 : texture_height - 1;
  if (*tex_x < min_x)
    *tex_x = min_x;
  if (*tex_y < min_y)
    *tex_y = min_y;
  if (*tex_x > max_x)
    *tex_x = max_x;
  if (*tex_y > max_y)
    *tex_y = max_y;
}

static inline uint16_t renderer_span_texture_power_of_two(
    const uint16_t *texture, int32_t texture_width, int32_t texture_height,
    int32_t texture_width_shift, int32_t texture_height_shift, int32_t u,
    int32_t v, int32_t z, bool transparent) {
  if (texture == NULL || texture_width <= 0 || texture_height <= 0)
    return TEXTURE_TRANSPARENT_COLOR;

  int32_t uv_x = renderer_span_restore_perspective_uv(u, z);
  int32_t uv_y = renderer_span_restore_perspective_uv(v, z);
  renderer_span_clamp_uv_fixed(&uv_x, &uv_y);

  int32_t tex_x = uv_x >> (SHIFT_FACTOR - texture_width_shift);
  int32_t tex_y = uv_y >> (SHIFT_FACTOR - texture_height_shift);
  renderer_span_clamp_texel_coords(texture_width, texture_height, &tex_x,
                                   &tex_y);

  int32_t x1 = tex_x + 1 < texture_width ? tex_x + 1 : tex_x;
  int32_t y1 = tex_y + 1 < texture_height ? tex_y + 1 : tex_y;
  int32_t row0 = tex_y << texture_width_shift;
  int32_t row1 = y1 << texture_width_shift;

  return renderer_span_sample_texture_2x2(texture, row0, row1, tex_x, x1,
                                           transparent);
}

static inline uint16_t renderer_span_texture_generic(
    const uint16_t *texture, int32_t texture_width, int32_t texture_height,
    int32_t u, int32_t v, int32_t z, bool transparent) {
  if (texture == NULL || texture_width <= 0 || texture_height <= 0)
    return TEXTURE_TRANSPARENT_COLOR;

  int32_t uv_x = renderer_span_restore_perspective_uv(u, z);
  int32_t uv_y = renderer_span_restore_perspective_uv(v, z);
  renderer_span_clamp_uv_fixed(&uv_x, &uv_y);

  int32_t tex_x = (uv_x * texture_width) >> SHIFT_FACTOR;
  int32_t tex_y = (uv_y * texture_height) >> SHIFT_FACTOR;
  renderer_span_clamp_texel_coords(texture_width, texture_height, &tex_x,
                                   &tex_y);

  int32_t x1 = tex_x + 1 < texture_width ? tex_x + 1 : tex_x;
  int32_t y1 = tex_y + 1 < texture_height ? tex_y + 1 : tex_y;
  int32_t row0 = tex_y * texture_width;
  int32_t row1 = y1 * texture_width;

  return renderer_span_sample_texture_2x2(texture, row0, row1, tex_x, x1,
                                           transparent);
}

#endif
