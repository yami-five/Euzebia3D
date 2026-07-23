#ifndef ILIGHTFACTORY_h
#define ILIGHTFACTORY_h

#include "light.h"
#include <stdint.h>

typedef struct {
  e3d_Light *(*create_point_light)(float x, float y, float z, float intensity,
                               uint16_t color);
  e3d_Light *(*create_directional_light)(float x, float y, float z, float intensity,
                                     uint16_t color);
} e3d_ILightFactory;

#endif