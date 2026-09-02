#include "renderer_platform.h"
#include "renderer_span.h"

void renderer_platform_init(const e3d_IHardware *hardware) { (void)hardware; }

e3d_TextureSource
renderer_platform_detect_texture_source(const uint16_t *texture) {
  (void)texture;
  return TEXTURE_SOURCE_UNKNOWN;
}

void renderer_platform_texture_span(
    uint16_t *dst, uint16_t length, const uint16_t *texture,
    int32_t texture_width, int32_t texture_height,
    int32_t texture_width_shift, int32_t texture_height_shift,
    bool transparent, int32_t u, int32_t du_dx, int32_t v, int32_t dv_dx,
    int32_t z, int32_t dz_dx) {
  bool use_power_of_two =
      texture_width_shift >= 0 && texture_height_shift >= 0 &&
      texture_width_shift <= SHIFT_FACTOR &&
      texture_height_shift <= SHIFT_FACTOR;

  if (use_power_of_two) {
    for (uint16_t i = 0; i < length; i++) {
      int32_t current_u = u >> UV_LERP_SHIFT;
      int32_t current_v = v >> UV_LERP_SHIFT;
      dst[i] = renderer_span_texture_power_of_two(
          texture, texture_width, texture_height, texture_width_shift,
          texture_height_shift, current_u, current_v, z, transparent);
      u += du_dx;
      v += dv_dx;
      z += dz_dx;
    }
  } else {
    for (uint16_t i = 0; i < length; i++) {
      int32_t current_u = u >> UV_LERP_SHIFT;
      int32_t current_v = v >> UV_LERP_SHIFT;
      dst[i] = renderer_span_texture_generic(texture, texture_width,
                                              texture_height, current_u,
                                              current_v, z, transparent);
      u += du_dx;
      v += dv_dx;
      z += dz_dx;
    }
  }
}

void renderer_platform_shade_span(uint16_t *dst, uint16_t length,
                                  const e3d_Light *light, int32_t light_value,
                                  int32_t light_delta) {
  e3d_RendererShadingContext context =
      renderer_span_make_shading_context(light);
  if (length <= MAX_SHADING_SPAN_LEN) {
    for (uint16_t i = 0; i < length; i++) {
      int32_t current_light = light_value >> LIGHT_LERP_SHIFT;
      dst[i] = renderer_span_shade_color(dst[i], &context, current_light);
      light_value += light_delta;
    }
  } else {
    int32_t first_pixel_light = light_value >> LIGHT_LERP_SHIFT;
    dst[0] = renderer_span_shade_color(dst[0], &context, first_pixel_light);
  }
}
