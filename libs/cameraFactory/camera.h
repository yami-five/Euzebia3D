#ifndef CAMERA_h
#define CAMERA_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "../storage/gfx.h"
#include "vectors.h"

typedef struct TransformInfo TransformInfo;

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

TransformInfo *add_camera_transformation(TransformInfo *currentTransformations, uint32_t *currentTransformationsNum, float w, float x, float y, float z, uint8_t transformationType);
void modify_camera_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex);
void camera_apply_transformations(Camera *camera);
/* transformationType:
   0 = translate, 1 = rotate, 2 = translateTarget, 3 = rotateTarget */
void rotate_camera(Camera *camera, float w, float x, float y, float z);
void translate_camera(Camera *camera, float x, float y, float z);
void rotate_camera_target(Camera *camera, float w, float x, float y, float z);
void translate_camera_target(Camera *camera, float x, float y, float z);

#endif
