#include "lightFactory.h"
#include "../storage/gfx.h"
#include "ILightFactory.h"
#include "fpa.h"
#include "light.h"
#include <stdlib.h>

e3d_Light *create_point_light(float x, float y, float z, float intensity,
                          uint16_t color) {
  e3d_Light *light = (e3d_Light *)malloc(sizeof(e3d_Light));
  if (light == NULL)
    return NULL;

  light->position.x = float_to_fixed(x);
  light->position.y = float_to_fixed(y);
  light->position.z = float_to_fixed(z);
  light->intensity = float_to_fixed(intensity);
  light->color = color;
  light->lightType = POINT_LIGHT;
  return light;
}

e3d_Light *create_directional_light(float x, float y, float z, float intensity,
                                uint16_t color) {
  e3d_Light *light = (e3d_Light *)malloc(sizeof(e3d_Light));
  if (light == NULL)
    return NULL;

  light->position.x = -float_to_fixed(x);
  light->position.y = -float_to_fixed(y);
  light->position.z = -float_to_fixed(z);
  light->intensity = float_to_fixed(intensity);
  light->color = color;
  light->lightType = DIRECTIONAL_LIGHT;
  return light;
}

static e3d_ILightFactory lightFactory = {
    .create_point_light = create_point_light,
    .create_directional_light = create_directional_light,
};

const e3d_ILightFactory *get_lightFactory(void) { return &lightFactory; }
