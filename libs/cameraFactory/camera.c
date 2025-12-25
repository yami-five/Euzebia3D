#include "camera.h"

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
    calculatePerspectiveMatrix(camera);
}

TransformInfo *add_camera_transformation(TransformInfo *currentTransformations, uint32_t *currentTransformationsNum, float w, float x, float y, float z, uint8_t transformationType)
{
    if (transformationType > 3)
        return currentTransformations;

    *currentTransformationsNum += 1;
    TransformInfo *newTransformations = (TransformInfo *)realloc(currentTransformations, *currentTransformationsNum * sizeof(TransformInfo));
    if (newTransformations == NULL)
    {
        *currentTransformationsNum -= 1;
        return currentTransformations;
    }
    newTransformations[*currentTransformationsNum - 1].transformVector = (TransformVector *)malloc(sizeof(TransformVector));
    newTransformations[*currentTransformationsNum - 1].transformType = transformationType;
    newTransformations[*currentTransformationsNum - 1].transformVector->w = float_to_fixed(w);
    newTransformations[*currentTransformationsNum - 1].transformVector->x = float_to_fixed(x);
    newTransformations[*currentTransformationsNum - 1].transformVector->y = float_to_fixed(y);
    newTransformations[*currentTransformationsNum - 1].transformVector->z = float_to_fixed(z);

    return newTransformations;
}

void modify_camera_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex)
{
    modify_transformation(currentTransformations, w, x, y, z, transformationIndex);
}

static void rotate_vector(Vector3 *resultVec, TransformVector *transVec)
{
    int32_t qt_rad = fixed_mul(transVec->w, PI2);
    int32_t c = fast_cos(qt_rad >> 1);
    int32_t s = fast_sin(qt_rad >> 1);
    Vector3 qVec = {
        .x = transVec->x,
        .y = transVec->y,
        .z = transVec->z};
    Quaternion q = {
        .w = c,
        .vec = &qVec};
    norm_vector(q.vec);
    mul_vec_scalar(q.vec, s);
    Vector3 qVecInv = {
        .x = -q.vec->x,
        .y = -q.vec->y,
        .z = -q.vec->z};
    Quaternion qInv = {
        .w = c,
        .vec = &qVecInv};
    Quaternion q_vertex = {
        .w = 0,
        .vec = resultVec};
    Quaternion *result = mul_quaternion(&q, &q_vertex);
    if (result == NULL || result->vec == NULL)
    {
        if (result != NULL)
        {
            free(result->vec);
            free(result);
        }
        return;
    }
    Quaternion *result2 = mul_quaternion(result, &qInv);
    if (result2 == NULL || result2->vec == NULL)
    {
        free(result->vec);
        free(result);
        if (result2 != NULL)
        {
            free(result2->vec);
            free(result2);
        }
        return;
    }
    resultVec->x = result2->vec->x;
    resultVec->y = result2->vec->y;
    resultVec->z = result2->vec->z;
    free(result->vec);
    free(result);
    free(result2->vec);
    free(result2);
}

static void translate_vector(Vector3 *resultVec, TransformVector *transVec)
{
    resultVec->x += transVec->x;
    resultVec->y += transVec->y;
    resultVec->z += transVec->z;
}

void camera_apply_transformations(Camera *camera)
{
    if (camera == NULL || camera->transformations == NULL || camera->transformationsNum == 0)
        return;

    for (uint32_t i = 0; i < camera->transformationsNum; i++)
    {
        TransformInfo *info = &camera->transformations[i];
        if (info->transformType == 0) /* translate */
            translate_vector(camera->pos, info->transformVector);
        if (info->transformType == 1) /* rotate */
        {
            rotate_vector(camera->pos, info->transformVector);
            rotate_vector(camera->up, info->transformVector);
        }
        if (info->transformType == 2) /* translateTarget */
            translate_vector(camera->target, info->transformVector);
        if (info->transformType == 3) /* rotateTarget */
        {
            Vector3 offset = {
                .x = camera->target->x - camera->pos->x,
                .y = camera->target->y - camera->pos->y,
                .z = camera->target->z - camera->pos->z,
            };
            rotate_vector(&offset, info->transformVector);
            camera->target->x = camera->pos->x + offset.x;
            camera->target->y = camera->pos->y + offset.y;
            camera->target->z = camera->pos->z + offset.z;
        }
    }
}
