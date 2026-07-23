#ifndef CAMERA_h
#define CAMERA_h

#include "../storage/gfx.h"
#include "fpa.h"
#include "stdio.h"
#include "transformations.h"
#include "vectors.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  e3d_Vector3 *pos;
  e3d_Vector3 *target;
  e3d_Vector3 *up;
  e3d_Vector3 *right;
  e3d_Vector3 *forward;
  int32_t *vMatrix;
  int32_t *pMatrix;
  e3d_TransformInfo *transformations;
  uint32_t transformationsNum;
} e3d_Camera;

typedef enum {
  CAMERA_TRANSFORM_TRANSLATE = 0,
  CAMERA_TRANSFORM_ROTATE = 1,
  CAMERA_TRANSFORM_TRANSLATE_TARGET = 2,
  CAMERA_TRANSFORM_ROTATE_TARGET = 3
} e3d_CameraTransformType;

e3d_TransformInfo *add_camera_transformation(e3d_TransformInfo *currentTransformations,
                                         uint32_t *currentTransformationsNum,
                                         float w, float x, float y, float z,
                                         uint8_t transformationType);
void modify_camera_transformation(e3d_TransformInfo *currentTransformations,
                                  float w, float x, float y, float z,
                                  uint32_t transformationIndex);
void camera_apply_transformations(e3d_Camera *camera);
void update_camera(e3d_Camera *camera);

#endif
