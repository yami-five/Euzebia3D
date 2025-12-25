#ifndef TRANSFORMATIONS_h
#define TRANSFORMATIONS_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "fpa.h"
#include "../storage/gfx.h"

typedef struct TransformVector
{
    int32_t w;
    int32_t x;
    int32_t y;
    int32_t z;
} TransformVector;

typedef struct TransformInfo
{
    uint8_t transformType;
    TransformVector *transformVector;
} TransformInfo;

void modify_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex);

#endif
