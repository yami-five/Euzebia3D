#ifndef ILIGHTFACTORY_h
#define ILIGHTFACTORY_h

#include <stdint.h>
#include "light.h"

typedef struct
{
    Light* (*create_point_light)(float x, float y, float z, float intensity, uint16_t color);
    Light* (*create_directional_light)(float x, float y, float z, float intensity, uint16_t color);
} ILightFactory;

#endif