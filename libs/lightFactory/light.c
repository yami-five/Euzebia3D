#include "light.h"

void set_light_pos(e3d_Light *light, float x, float y, float z) {
  if (light == NULL)
    return;

  int32_t directionMultiplier =
      light->lightType == DIRECTIONAL_LIGHT ? -1 : 1;
  light->position.x = directionMultiplier * float_to_fixed(x);
  light->position.y = directionMultiplier * float_to_fixed(y);
  light->position.z = directionMultiplier * float_to_fixed(z);
}

void set_light_color(e3d_Light *light, uint16_t newColor) {
  if (light == NULL)
    return;
  light->color = newColor;
}

void set_light_intensity(e3d_Light *light, float newIntensity) {
  if (light == NULL)
    return;
  light->intensity = float_to_fixed(newIntensity);
}

void free_light(e3d_Light *light) { free(light); }
