#ifndef LIGHT_h
#define LIGHT_h

#include "../storage/gfx.h"
#include "stdio.h"
#include "vectors.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  POINT_LIGHT = 0,
  DIRECTIONAL_LIGHT = 1,
} e3d_LightType;

typedef struct {
  e3d_Vector3 position;
  uint32_t intensity;
  uint16_t color;
  e3d_LightType lightType;
} e3d_Light;

void set_light_pos(e3d_Light *light, float x, float y, float z);
void set_light_color(e3d_Light *light, uint16_t newColor);
void set_light_intensity(e3d_Light *light, float newIntensity);
void free_light(e3d_Light *light);

#endif
