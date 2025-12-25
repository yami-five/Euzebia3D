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

void mul_quaternion(Quaternion *out, const Quaternion *q1, const Quaternion *q2);
void add_vectors(Vector3 *out, const Vector3 *vecA, const Vector3 *vecB);
void sub_vectors(Vector3 *out, const Vector3 *vecA, const Vector3 *vecB);
void mul_vectors(Vector3 *out, const Vector3 *vecA, const Vector3 *vecB);
void mul_vec_scalar(Vector3 *vec, const int32_t scal);
int32_t dot_product(const Vector3 *vecA, const Vector3 *vecB);
int32_t len_vector(const Vector3 *vec);
void norm_vector(Vector3 *vec);
void fixed_mul_matrix_vector(int32_t *x, int32_t *y, int32_t *z, int32_t *w, int32_t *matrix);
int * mul_matrices(int *matrix1, int *matrix, uint8_t w, uint8_t h);

#endif
