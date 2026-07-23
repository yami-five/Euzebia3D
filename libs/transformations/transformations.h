#ifndef TRANSFORMATIONS_h
#define TRANSFORMATIONS_h

#include "../storage/gfx.h"
#include "fpa.h"
#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct e3d_TransformVector {
  int32_t w;
  int32_t x;
  int32_t y;
  int32_t z;
} e3d_TransformVector;

typedef enum {
  MODEL_TRANSFORM_ROTATE = 0,
  MODEL_TRANSFORM_TRANSLATE = 1,
  MODEL_TRANSFORM_SCALE = 2
} e3d_ModelTransformType;

typedef struct e3d_TransformInfo {
  uint8_t transformType;
  e3d_TransformVector *transformVector;
} e3d_TransformInfo;

void modify_transformation(e3d_TransformInfo *currentTransformations, float w,
                           float x, float y, float z,
                           uint32_t transformationIndex);

#endif
