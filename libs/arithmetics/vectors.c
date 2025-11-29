#include "vectors.h"
#include "fpa.h"
#include <stdlib.h>
#include <stdio.h>

void mul_quaternion_inline(const Quaternion *q1, const Quaternion *q2, Quaternion *out)
{
    if (!q1 || !q2 || !out || !q1->vec || !q2->vec || !out->vec)
        return;
    out->w = fixed_mul(q1->w, q2->w) - fixed_mul(q1->vec->x, q2->vec->x) - fixed_mul(q1->vec->y, q2->vec->y) - fixed_mul(q1->vec->z, q2->vec->z);
    out->vec->x = fixed_mul(q1->w, q2->vec->x) + fixed_mul(q1->vec->x, q2->w) - fixed_mul(q1->vec->y, q2->vec->z) + fixed_mul(q1->vec->z, q2->vec->y);
    out->vec->y = fixed_mul(q1->w, q2->vec->y) + fixed_mul(q1->vec->x, q2->vec->z) + fixed_mul(q1->vec->y, q2->w) - fixed_mul(q1->vec->z, q2->vec->x);
    out->vec->z = fixed_mul(q1->w, q2->vec->z) - fixed_mul(q1->vec->x, q2->vec->y) + fixed_mul(q1->vec->y, q2->vec->x) + fixed_mul(q1->vec->z, q2->w);
}

Quaternion *mul_quaternion(const Quaternion *q1, const Quaternion *q2)
{
    Quaternion *result = (Quaternion *)malloc(sizeof(Quaternion));
    if (result == NULL)
        return NULL;
    result->vec = (Vector3 *)malloc(sizeof(Vector3));
    if (result->vec == NULL)
    {
        free(result);
        return NULL;
    }
    mul_quaternion_inline(q1, q2, result);
    return result;
}

Vector3 *add_vectors(Vector3 *vecA, Vector3 *vecB)
{
    Vector3 *result = (Vector3 *)malloc(sizeof(Vector3));
    result->x = vecA->x + vecB->x;
    result->y = vecA->y + vecB->y;
    result->z = vecA->z + vecB->z;
    return result;
}

Vector3 *sub_vectors(Vector3 *vecA, Vector3 *vecB)
{
    Vector3 *result = (Vector3 *)malloc(sizeof(Vector3));
    result->x = vecA->x - vecB->x;
    result->y = vecA->y - vecB->y;
    result->z = vecA->z - vecB->z;
    return result;
}

Vector3 *mul_vectors(Vector3 *vecA, Vector3 *vecB)
{
    Vector3 *result = (Vector3 *)malloc(sizeof(Vector3));
    result->x = fixed_mul(vecA->y, vecB->z) - fixed_mul(vecA->z, vecB->y);
    result->y = fixed_mul(vecA->z, vecB->x) - fixed_mul(vecA->x, vecB->z);
    result->z = fixed_mul(vecA->x, vecB->y) - fixed_mul(vecA->y, vecB->x);
    return result;
}

int32_t dot_product(Vector3 *vecA, Vector3 *vecB)
{
    return fixed_mul(vecA->x, vecB->x) + fixed_mul(vecA->y, vecB->y) + fixed_mul(vecA->z, vecB->z);
}
void add_vec_scalar(Vector3 *vec, int32_t scal)
{
    vec->x = vec->x + scal;
    vec->y = vec->y + scal;
    vec->z = vec->z + scal;
}

void sub_vec_scalar(Vector3 *vec, int32_t scal)
{
    vec->x = vec->x - scal;
    vec->y = vec->y - scal;
    vec->z = vec->z - scal;
}

void mul_vec_scalar(Vector3 *vec, int32_t scal)
{
    vec->x = fixed_mul(vec->x, scal);
    vec->y = fixed_mul(vec->y, scal);
    vec->z = fixed_mul(vec->z, scal);
}

int32_t len_vector(Vector3 *vec)
{
    return fast_sqrt(fixed_pow(vec->x) + fixed_pow(vec->y) + fixed_pow(vec->z));
}

void norm_vector(Vector3 *vec)
{
    int32_t len = len_vector(vec);
    if (len == 0)
        return;
    vec->x = fixed_div(vec->x, len);
    vec->y = fixed_div(vec->y, len);
    vec->z = fixed_div(vec->z, len);
}

void fixed_mul_matrix_vector(int32_t *x, int32_t *y, int32_t *z, int32_t *w, int32_t *matrix)
{
    int32_t resultX = 0;
    int32_t resultY = 0;
    int32_t resultZ = 0;
    int32_t resultW = 0;

    resultX += fixed_mul(*x, matrix[0]) + fixed_mul(*y, matrix[1]) + fixed_mul(*z, matrix[2]) + fixed_mul(*w, matrix[3]);
    resultY += fixed_mul(*x, matrix[4]) + fixed_mul(*y, matrix[5]) + fixed_mul(*z, matrix[6]) + fixed_mul(*w, matrix[7]);
    resultZ += fixed_mul(*x, matrix[8]) + fixed_mul(*y, matrix[9]) + fixed_mul(*z, matrix[10]) + fixed_mul(*w, matrix[11]);
    resultW += fixed_mul(*x, matrix[12]) + fixed_mul(*y, matrix[13]) + fixed_mul(*z, matrix[14]) + fixed_mul(*w, matrix[15]);

    *x = resultX;
    *y = resultY;
    *z = resultZ;
    *w = resultW;
}

int *mul_matrices(int *matrix1, int *matrix2, uint8_t w, uint8_t h)
{
    int *result = (int *)malloc(sizeof(int) * w * h);
    for (uint8_t i = 0; i < h; i++)
    {
        for (uint8_t j = 0; j < w; j++)
        {
            int sum = 0;
            for (uint8_t k=0; k<w; k++)
            {
                sum += fixed_mul(matrix1[i*w+k],matrix2[k*w+j]);
            }
            result[i*w+j]=sum;
        }
    }
    return result;
}
