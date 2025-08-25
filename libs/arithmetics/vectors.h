#include "fpa.h"

#ifndef VECTORS_H
#define VECTORS_H

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t z;
} Vector3;

typedef struct
{
    int32_t w;
    Vector3 *vec;
} Quaternion;

typedef struct
{
    int32_t x;
    int32_t y;
} Vector2;

Quaternion *mul_quaternion(Quaternion *q1,Quaternion *q2);
Vector3 *add_vectors(Vector3 *vecA, Vector3 *vecB);
Vector3 *sub_vectors(Vector3 *vecA, Vector3 *vecB);
Vector3 *mul_vectors(Vector3 *vecA, Vector3 *vecB);
void mul_vec_scalar(Vector3 *vec, int32_t scal);
int32_t dot_product(Vector3 *vecA, Vector3 *vecB);
int32_t len_vector(Vector3 *vec);
void norm_vector(Vector3 *vec);
void fixed_mul_matrix_vector(int32_t *x, int32_t *y, int32_t *z, int32_t *w, int32_t *matrix);
int * mul_matrices(int *matrix1, int *matrix, uint8_t w, uint8_t h);

#endif