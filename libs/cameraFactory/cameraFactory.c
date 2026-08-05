#include "cameraFactory.h"
#include "../storage/gfx.h"
#include "ICameraFactory.h"
#include "camera.h"
#include "fpa.h"
#include <stdlib.h>

e3d_Camera *create_camera(float camX, float camY, float camZ, float targetX,
                      float targetY, float targetZ, float upX, float upY,
                      float upZ) {
  e3d_Camera *cam = (e3d_Camera *)calloc(1, sizeof(e3d_Camera));
  if (cam == NULL)
    return NULL;

  cam->pos = (e3d_Vector3 *)malloc(sizeof(e3d_Vector3));
  cam->target = (e3d_Vector3 *)malloc(sizeof(e3d_Vector3));
  cam->up = (e3d_Vector3 *)malloc(sizeof(e3d_Vector3));
  cam->forward = (e3d_Vector3 *)malloc(sizeof(e3d_Vector3));
  cam->right = (e3d_Vector3 *)malloc(sizeof(e3d_Vector3));
  cam->vMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
  cam->pMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
  if (cam->pos == NULL || cam->target == NULL || cam->up == NULL ||
      cam->forward == NULL || cam->right == NULL || cam->vMatrix == NULL ||
      cam->pMatrix == NULL) {
    free_camera(cam);
    return NULL;
  }

  cam->pos->x = float_to_fixed(camX);
  cam->pos->y = float_to_fixed(camY);
  cam->pos->z = float_to_fixed(camZ);
  cam->target->x = float_to_fixed(targetX);
  cam->target->y = float_to_fixed(targetY);
  cam->target->z = float_to_fixed(targetZ);
  cam->up->x = float_to_fixed(upX);
  cam->up->y = float_to_fixed(upY);
  cam->up->z = float_to_fixed(upZ);
  update_camera(cam);
  return cam;
}

static e3d_ICameraFactory cameraFactory = {.create_camera = create_camera};

const e3d_ICameraFactory *get_cameraFactory(void) { return &cameraFactory; }
