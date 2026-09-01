#include "renderer_platform.h"
#include "renderer_span.h"

#include "hardware/interp.h"

void renderer_platform_init(const e3d_IHardware *hardware) {
  (void)hardware;

  interp_config uv_config = interp_default_config();
  interp_config_set_signed(&uv_config, true);
  interp_set_config(interp0, 0, &uv_config);
  interp_set_config(interp0, 1, &uv_config);

  interp_config z_config = interp_default_config();
  interp_config_set_signed(&z_config, true);
  interp_set_config(interp1, 0, &z_config);

  interp_config light_config = interp_default_config();
  interp_config_set_signed(&light_config, true);
  interp_set_config(interp1, 1, &light_config);

  interp_set_base(interp0, 0, 0);
  interp_set_base(interp0, 1, 0);
  interp_set_base(interp0, 2, 0);
  interp_set_base(interp1, 0, 0);
  interp_set_base(interp1, 1, 0);
  interp_set_base(interp1, 2, 0);
}

e3d_TextureSource
renderer_platform_detect_texture_source(const uint16_t *texture) {
  uintptr_t address = (uintptr_t)texture;
  uint32_t region = (uint32_t)(address & 0xff000000u);
  if (region == 0x10000000u)
    return TEXTURE_SOURCE_FLASH;
  if (region == 0x11000000u)
    return TEXTURE_SOURCE_PSRAM;
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

  interp_set_accumulator(interp0, 0, (uint32_t)u);
  interp_set_accumulator(interp0, 1, (uint32_t)v);
  interp_set_accumulator(interp1, 0, (uint32_t)z);

  if (use_power_of_two) {
    for (uint16_t i = 0; i < length; i++) {
      int32_t current_u =
          ((int32_t)interp_get_accumulator(interp0, 0)) >> UV_LERP_SHIFT;
      int32_t current_v =
          ((int32_t)interp_get_accumulator(interp0, 1)) >> UV_LERP_SHIFT;
      int32_t current_z = (int32_t)interp_get_accumulator(interp1, 0);
      dst[i] = renderer_span_texture_power_of_two(
          texture, texture_width, texture_height, texture_width_shift,
          texture_height_shift, current_u, current_v, current_z, transparent);

      interp_add_accumulator(interp0, 0, (uint32_t)du_dx);
      interp_add_accumulator(interp0, 1, (uint32_t)dv_dx);
      interp_add_accumulator(interp1, 0, (uint32_t)dz_dx);
    }
  } else {
    for (uint16_t i = 0; i < length; i++) {
      int32_t current_u =
          ((int32_t)interp_get_accumulator(interp0, 0)) >> UV_LERP_SHIFT;
      int32_t current_v =
          ((int32_t)interp_get_accumulator(interp0, 1)) >> UV_LERP_SHIFT;
      int32_t current_z = (int32_t)interp_get_accumulator(interp1, 0);
      dst[i] = renderer_span_texture_generic(
          texture, texture_width, texture_height, current_u, current_v,
          current_z, transparent);

      interp_add_accumulator(interp0, 0, (uint32_t)du_dx);
      interp_add_accumulator(interp0, 1, (uint32_t)dv_dx);
      interp_add_accumulator(interp1, 0, (uint32_t)dz_dx);
    }
  }
}

void renderer_platform_shade_span(uint16_t *dst, uint16_t length,
                                  const e3d_Light *light, int32_t light_value,
                                  int32_t light_delta) {
  e3d_RendererShadingContext context =
      renderer_span_make_shading_context(light);
  if (length <= MAX_SHADING_SPAN_LEN) {
    interp_set_accumulator(interp1, 1, (uint32_t)light_value);

    for (uint16_t i = 0; i < length; i++) {
      int32_t current_light =
          ((int32_t)interp_get_accumulator(interp1, 1)) >> LIGHT_LERP_SHIFT;
      dst[i] = renderer_span_shade_color(dst[i], &context, current_light);
      interp_add_accumulator(interp1, 1, (uint32_t)light_delta);
    }
  } else {
    int32_t first_pixel_light = light_value >> LIGHT_LERP_SHIFT;
    dst[0] = renderer_span_shade_color(dst[0], &context, first_pixel_light);
  }
}
