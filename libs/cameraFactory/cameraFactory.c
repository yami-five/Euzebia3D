#include "ICameraFactory.h"
#include "cameraFactory.h"
#include "camera.h"
#include "fpa.h"
#include "../storage/gfx.h"
#include <stdlib.h>


Camera *create_camera(float camX, float camY, float camZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ)
{
    Camera *cam = (Camera *)malloc(sizeof(Camera));
    cam->pos = (Vector3 *)malloc(sizeof(Vector3));
    cam->pos->x = float_to_fixed(camX);
    cam->pos->y = float_to_fixed(camY);
    cam->pos->z = float_to_fixed(camZ);
    cam->target = (Vector3 *)malloc(sizeof(Vector3));
    cam->target->x = float_to_fixed(targetX);
    cam->target->y = float_to_fixed(targetY);
    cam->target->z = float_to_fixed(targetZ);
    cam->up = (Vector3 *)malloc(sizeof(Vector3));
    cam->up->x = float_to_fixed(upX);
    cam->up->y = float_to_fixed(upY);
    cam->up->z = float_to_fixed(upZ);
    cam->forward = (Vector3 *)malloc(sizeof(Vector3));
    cam->right = (Vector3 *)malloc(sizeof(Vector3));
    cam->vMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
    cam->pMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
    cam->transformations = NULL;
    cam->transformationsNum = 0;
    update_camera(cam);
    return cam;
}

static ICameraFactory camera = {
    .create_camera = create_camera
};

const ICameraFactory *get_cameraFactory(void)
{
    return &camera;
}
