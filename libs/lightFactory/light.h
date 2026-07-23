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

#endif