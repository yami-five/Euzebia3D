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

typedef enum
{
    MODEL_TRANSFORM_ROTATE = 0,
    MODEL_TRANSFORM_TRANSLATE = 1,
    MODEL_TRANSFORM_SCALE = 2
} ModelTransformType;

typedef struct TransformInfo
{
    uint8_t transformType;
    TransformVector *transformVector;
} TransformInfo;

void modify_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex);

#endif
