#include "ICameraFactory.h"
#include "cameraFactory.h"
#include "camera.h"
#include "fpa.h"
#include "../storage/gfx.h"
#include <stdlib.h>

static void free_partial_camera(Camera *cam)
{
    if (cam == NULL)
        return;
    free(cam->pos);
    free(cam->target);
    free(cam->up);
    free(cam->forward);
    free(cam->right);
    free(cam->vMatrix);
    free(cam->pMatrix);
    free(cam);
}

Camera *create_camera(float camX, float camY, float camZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ)
{
    Camera *cam = (Camera *)calloc(1, sizeof(Camera));
    if (cam == NULL)
        return NULL;

    cam->pos = (Vector3 *)malloc(sizeof(Vector3));
    cam->target = (Vector3 *)malloc(sizeof(Vector3));
    cam->up = (Vector3 *)malloc(sizeof(Vector3));
    cam->forward = (Vector3 *)malloc(sizeof(Vector3));
    cam->right = (Vector3 *)malloc(sizeof(Vector3));
    cam->vMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
    cam->pMatrix = (int32_t *)malloc(sizeof(int32_t) * 16);
    if (cam->pos == NULL || cam->target == NULL || cam->up == NULL || cam->forward == NULL || cam->right == NULL || cam->vMatrix == NULL || cam->pMatrix == NULL)
    {
        free_partial_camera(cam);
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

static ICameraFactory camera = {
    .create_camera = create_camera
};

const ICameraFactory *get_cameraFactory(void)
{
    return &camera;
}
