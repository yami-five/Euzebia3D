#ifndef CAMERA_h
#define CAMERA_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "../shared/gfx.h"
#include "vectors.h"

typedef struct
{
    Vector3 *pos;
    Vector3 *target;
    Vector3 *up;
    Vector3 *right;
    Vector3 *forward;
    int32_t *vMatrix;
    int32_t *pMatrix;
} Camera;

#endif