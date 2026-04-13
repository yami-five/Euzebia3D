#ifndef CAMERA_h
#define CAMERA_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "../storage/gfx.h"
#include "vectors.h"
#include "transformations.h"
#include "fpa.h"

typedef struct
{
    Vector3 *pos;
    Vector3 *target;
    Vector3 *up;
    Vector3 *right;
    Vector3 *forward;
    int32_t *vMatrix;
    int32_t *pMatrix;
    TransformInfo *transformations;
    uint32_t transformationsNum;
} Camera;

typedef enum
{
    CAMERA_TRANSFORM_TRANSLATE = 0,
    CAMERA_TRANSFORM_ROTATE = 1,
    CAMERA_TRANSFORM_TRANSLATE_TARGET = 2,
    CAMERA_TRANSFORM_ROTATE_TARGET = 3
} CameraTransformType;

TransformInfo *add_camera_transformation(TransformInfo *currentTransformations, uint32_t *currentTransformationsNum, float w, float x, float y, float z, uint8_t transformationType);
void modify_camera_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex);
void camera_apply_transformations(Camera *camera);
void update_camera(Camera *camera);

#endif
