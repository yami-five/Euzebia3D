#include "camera.h"
#include "mesh.h"
#include "fpa.h"

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

static void rotate_vectors(Vector3 *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    int32_t qt_rad = fixed_mul(vector->w, PI2);
    int32_t c = fast_cos(qt_rad >> 1);
    int32_t s = fast_sin(qt_rad >> 1);
    Vector3 qVec = {
        .x = vector->x,
        .y = vector->y,
        .z = vector->z};
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
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        Quaternion q_vertex = {
            .w = 0,
            .vec = &vertices[i]};
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
        vertices[i].x = result2->vec->x;
        vertices[i].y = result2->vec->y;
        vertices[i].z = result2->vec->z;
        free(result->vec);
        free(result);
        free(result2->vec);
        free(result2);
    }
}

static void translate_vectors(Vector3 *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i].x += vector->x;
        vertices[i].y += vector->y;
        vertices[i].z += vector->z;
    }
}

static void scale_vectors(Vector3 *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i].x = fixed_mul(vertices[i].x, vector->x);
        vertices[i].y = fixed_mul(vertices[i].y, vector->y);
        vertices[i].z = fixed_mul(vertices[i].z, vector->z);
    }
}

static void rotate_vector(Vector3 *vec, float w, float x, float y, float z)
{
    TransformVector vector = {
        .w = float_to_fixed(w),
        .x = float_to_fixed(x),
        .y = float_to_fixed(y),
        .z = float_to_fixed(z),
    };
    rotate_vectors(vec, 1, &vector);
}

static void translate_vector(Vector3 *vec, float x, float y, float z)
{
    vec->x += float_to_fixed(x);
    vec->y += float_to_fixed(y);
    vec->z += float_to_fixed(z);
}

void camera_apply_transformations(Camera *camera)
{
    if (camera == NULL || camera->transformations == NULL || camera->transformationsNum == 0)
        return;

    for (uint32_t i = 0; i < camera->transformationsNum; i++)
    {
        TransformInfo *info = &camera->transformations[i];
        if (info->transformType == 0) /* translate */
            translate_vectors(camera->pos, 1, info->transformVector);
        if (info->transformType == 1) /* rotate */
        {
            rotate_vectors(camera->pos, 1, info->transformVector);
            rotate_vectors(camera->up, 1, info->transformVector);
        }
        if (info->transformType == 2) /* translateTarget */
            translate_vectors(camera->target, 1, info->transformVector);
        if (info->transformType == 3) /* rotateTarget */
        {
            Vector3 offset = {
                .x = camera->target->x - camera->pos->x,
                .y = camera->target->y - camera->pos->y,
                .z = camera->target->z - camera->pos->z,
            };
            rotate_vectors(&offset, 1, info->transformVector);
            camera->target->x = camera->pos->x + offset.x;
            camera->target->y = camera->pos->y + offset.y;
            camera->target->z = camera->pos->z + offset.z;
        }
    }
}

void rotate_camera(Camera *camera, float w, float x, float y, float z)
{
    if (camera == NULL || camera->pos == NULL || camera->up == NULL)
        return;
    rotate_vector(camera->pos, w, x, y, z);
    rotate_vector(camera->up, w, x, y, z);
}

void translate_camera(Camera *camera, float x, float y, float z)
{
    if (camera == NULL || camera->pos == NULL)
        return;
    translate_vector(camera->pos, x, y, z);
}

void rotate_camera_target(Camera *camera, float w, float x, float y, float z)
{
    if (camera == NULL || camera->target == NULL || camera->pos == NULL)
        return;
    Vector3 offset = {
        .x = camera->target->x - camera->pos->x,
        .y = camera->target->y - camera->pos->y,
        .z = camera->target->z - camera->pos->z,
    };
    rotate_vector(&offset, w, x, y, z);
    camera->target->x = camera->pos->x + offset.x;
    camera->target->y = camera->pos->y + offset.y;
    camera->target->z = camera->pos->z + offset.z;
}

void translate_camera_target(Camera *camera, float x, float y, float z)
{
    if (camera == NULL || camera->target == NULL)
        return;
    translate_vector(camera->target, x, y, z);
}
