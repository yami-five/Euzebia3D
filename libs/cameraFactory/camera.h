#ifndef CAMERA_h
#define CAMERA_h

#include "../storage/gfx.h"
#include "fpa.h"
#include "stdio.h"
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
} e3d_Camera;

void set_camera_pos(e3d_Camera *camera, float x, float y, float z);
void set_camera_target_pos(e3d_Camera *camera, float x, float y, float z);
void update_camera(e3d_Camera *camera);
void free_camera(e3d_Camera *camera);

#endif
