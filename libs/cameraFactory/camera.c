#include "camera.h"

#define ZNEAR 4096       // floatToFixed(1.0f)
#define ZFAR 409600      // floatToFixed(100.0f)
#define ASPECTRATIO 4096 // 1.0; the renderer outputs square display pixels
#define TANFOV2 4096     // tan(fov/2) fov=90

static void calculate_forward_vector(e3d_Vector3 *out, const e3d_Vector3 *pos,
                                     const e3d_Vector3 *target) {
  sub_vectors(out, pos, target);
  norm_vector(out);
}

static void calculate_right_vector(e3d_Vector3 *out, const e3d_Vector3 *up,
                                   const e3d_Vector3 *forward) {
  mul_vectors(out, up, forward);
  norm_vector(out);
}

static void calculate_up_vector(e3d_Vector3 *out, const e3d_Vector3 *forward,
                                const e3d_Vector3 *right) {
  mul_vectors(out, forward, right);
  norm_vector(out);
}

void calculateViewMatrix(e3d_Camera *camera) {
  camera->vMatrix[0] = camera->right->x;
  camera->vMatrix[1] = camera->right->y;
  camera->vMatrix[2] = camera->right->z;
  camera->vMatrix[3] = -dot_product(camera->pos, camera->right);
  camera->vMatrix[4] = camera->up->x;
  camera->vMatrix[5] = camera->up->y;
  camera->vMatrix[6] = camera->up->z;
  camera->vMatrix[7] = -dot_product(camera->pos, camera->up);
  camera->vMatrix[8] = camera->forward->x;
  camera->vMatrix[9] = camera->forward->y;
  camera->vMatrix[10] = camera->forward->z;
  camera->vMatrix[11] = -dot_product(camera->pos, camera->forward);
  camera->vMatrix[12] = camera->vMatrix[13] = camera->vMatrix[14] = 0;
  camera->vMatrix[15] = SCALE_FACTOR;
}

void calculatePerspectiveMatrix(e3d_Camera *camera) {
  // Screen-space projection currently treats the perspective-divide result as
  // a pixel offset directly. Applying the 4:3 viewport aspect here therefore
  // compressed X and produced visibly rectangular pixels. Keep the horizontal
  // and vertical projection scales equal for square pixels.
  camera->pMatrix[0] = fixed_div(SCALE_FACTOR, fixed_mul(TANFOV2, ASPECTRATIO));
  camera->pMatrix[1] = camera->pMatrix[2] = camera->pMatrix[3] = 0;
  camera->pMatrix[4] = 0;
  camera->pMatrix[5] = fixed_div(SCALE_FACTOR, TANFOV2);
  camera->pMatrix[6] = camera->pMatrix[7] = 0;
  camera->pMatrix[8] = camera->pMatrix[9] = 0;
  camera->pMatrix[10] = -fixed_div((ZFAR + ZNEAR), (ZFAR - ZNEAR));
  camera->pMatrix[11] = -SCALE_FACTOR;
  camera->pMatrix[12] = camera->pMatrix[13] = 0;
  camera->pMatrix[14] = -fixed_div(2 * fixed_mul(ZFAR, ZNEAR), (ZFAR - ZNEAR));
  ;
  camera->pMatrix[15] = 0;
}

void set_camera_pos(e3d_Camera *camera, float x, float y, float z) {
  camera->pos->x = float_to_fixed(x);
  camera->pos->y = float_to_fixed(y);
  camera->pos->z = float_to_fixed(z);
}

void set_camera_target_pos(e3d_Camera *camera, float x, float y, float z) {
  camera->target->x = float_to_fixed(x);
  camera->target->y = float_to_fixed(y);
  camera->target->z = float_to_fixed(z);
}

void update_camera(e3d_Camera *camera) {
  if (camera == NULL || camera->pos == NULL || camera->target == NULL ||
      camera->up == NULL || camera->right == NULL || camera->forward == NULL)
    return;
  calculate_forward_vector(camera->forward, camera->pos, camera->target);
  calculate_right_vector(camera->right, camera->up, camera->forward);
  calculate_up_vector(camera->up, camera->forward, camera->right);
  calculateViewMatrix(camera);
  calculatePerspectiveMatrix(camera);
}

void free_camera(e3d_Camera *camera) {
  if (camera == NULL)
    return;

  free(camera->pos);
  free(camera->target);
  free(camera->up);
  free(camera->forward);
  free(camera->right);
  free(camera->vMatrix);
  free(camera->pMatrix);
  free(camera);
}
