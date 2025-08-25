#ifndef LIGHT_h
#define LIGHT_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "../shared/gfx.h"
#include "vectors.h"

typedef struct
{
    Vector3 position;
    uint32_t intensity;
    uint16_t color;
} PointLight;

#endif