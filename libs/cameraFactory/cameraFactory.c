#include "ICameraFactory.h"
#include "cameraFactory.h"
#include "camera.h"
#include "fpa.h"
#include "../storage/gfx.h"
#include <stdlib.h>

#define ZNEAR 4096       // floatToFixed(1.0f)
#define ZFAR 409600      // floatToFixed(100.0f)
#define ASPECTRATIO 4096 // 1:1
#define TANFOV2 6634     // tan(fov/2)

static void calculate_forward_vector(Vector3 *out, const Vector3 *pos, const Vector3 *target)
{
    out->x = pos->x - target->x;
    out->y = pos->y - target->y;
    out->z = pos->z - target->z;
    norm_vector(out);
}

static void calculate_right_vector(Vector3 *out, const Vector3 *up, const Vector3 *forward)
{
    out->x = fixed_mul(up->y, forward->z) - fixed_mul(up->z, forward->y);
    out->y = fixed_mul(up->z, forward->x) - fixed_mul(up->x, forward->z);
    out->z = fixed_mul(up->x, forward->y) - fixed_mul(up->y, forward->x);
    norm_vector(out);
}

static void calculate_up_vector(Vector3 *out, const Vector3 *forward, const Vector3 *right)
{
    out->x = fixed_mul(forward->y, right->z) - fixed_mul(forward->z, right->y);
    out->y = fixed_mul(forward->z, right->x) - fixed_mul(forward->x, right->z);
    out->z = fixed_mul(forward->x, right->y) - fixed_mul(forward->y, right->x);
    norm_vector(out);
}

void calculateViewMatrix(Camera *camera)
{
    camera->vMatrix[0] = camera->right->x;
    camera->vMatrix[1] = camera->right->y;
    camera->vMatrix[2] = camera->right->z;
    camera->vMatrix[3] = -dot_product(camera->right, camera->pos);
    camera->vMatrix[4] = camera->up->x;
    camera->vMatrix[5] = camera->up->y;
    camera->vMatrix[6] = camera->up->z;
    camera->vMatrix[7] = -dot_product(camera->up, camera->pos);
    camera->vMatrix[8] = camera->forward->x;
    camera->vMatrix[9] = camera->forward->y;
    camera->vMatrix[10] = camera->forward->z;
    camera->vMatrix[11] = -dot_product(camera->forward, camera->pos);
    camera->vMatrix[12] =
        camera->vMatrix[13] =
            camera->vMatrix[14] = 0;
    camera->vMatrix[15] = SCALE_FACTOR;
}

void calculatePerspectiveMatrix(Camera *camera)
{
    camera->pMatrix[0] = fixed_div(SCALE_FACTOR, fixed_mul(TANFOV2, ASPECTRATIO));
    camera->pMatrix[1] =
        camera->pMatrix[2] =
            camera->pMatrix[3] = 0;
    camera->pMatrix[4] = 0;
    camera->pMatrix[5] = fixed_div(SCALE_FACTOR, TANFOV2);
    camera->pMatrix[6] =
        camera->pMatrix[7] = 0;
    camera->pMatrix[8] =
        camera->pMatrix[9] = 0;
    camera->pMatrix[10] = -fixed_div((ZFAR + ZNEAR), (ZFAR - ZNEAR));
    camera->pMatrix[11] = -fixed_div(2 * fixed_mul(ZFAR, ZNEAR), (ZFAR - ZNEAR));
    camera->pMatrix[12] =
        camera->pMatrix[13] = 0;
    camera->pMatrix[14] = -SCALE_FACTOR;
    camera->pMatrix[15] = 0;
}

void update_camera(Camera *camera)
{
    if (camera == NULL || camera->pos == NULL || camera->target == NULL || camera->up == NULL || camera->right == NULL || camera->forward == NULL)
        return;
    camera_apply_transformations(camera);
    calculate_forward_vector(camera->forward, camera->pos, camera->target);
    calculate_right_vector(camera->right, camera->up, camera->forward);
    calculate_up_vector(camera->up, camera->forward, camera->right);
    calculateViewMatrix(camera);
}

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
    calculatePerspectiveMatrix(cam);
    return cam;
}

static ICameraFactory camera = {
    .create_camera = create_camera,
    .update_camera = update_camera,
};

const ICameraFactory *get_cameraFactory(void)
{
    return &camera;
}
