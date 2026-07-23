#include "fpa.h"

#ifndef VECTORS_H
#define VECTORS_H

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} e3d_Vector3;

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
  int32_t w;
} e3d_Vector4;

typedef struct {
  int32_t w;
  e3d_Vector3 *vec;
} e3d_Quaternion;

typedef struct {
  int32_t x;
  int32_t y;
} e3d_Vector2;

void mul_quaternion(e3d_Quaternion *out, const e3d_Quaternion *q1,
                    const e3d_Quaternion *q2);
void add_vectors(e3d_Vector3 *out, const e3d_Vector3 *vecA, const e3d_Vector3 *vecB);
void sub_vectors(e3d_Vector3 *out, const e3d_Vector3 *vecA, const e3d_Vector3 *vecB);
void mul_vectors(e3d_Vector3 *out, const e3d_Vector3 *vecA, const e3d_Vector3 *vecB);
void mul_vec_scalar(e3d_Vector3 *vec, const int32_t scal);
int32_t dot_product(const e3d_Vector3 *vecA, const e3d_Vector3 *vecB);
int32_t len_vector(const e3d_Vector3 *vec);
void norm_vector(e3d_Vector3 *vec);
void fixed_mul_matrix_vector(int32_t *x, int32_t *y, int32_t *z, int32_t *w,
                             int32_t *matrix);
int *mul_matrices(int *matrix1, int *matrix, uint8_t w, uint8_t h);

#endif
