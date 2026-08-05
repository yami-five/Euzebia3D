#ifndef PRIMITIVES_h
#define PRIMITIVES_h

#include "material.h"
#include "vectors.h"
#include <stdint.h>

typedef struct {
  e3d_Vector4 a;
  e3d_Vector4 b;
  e3d_Vector4 c;
  e3d_Vector2 uvA;
  e3d_Vector2 uvB;
  e3d_Vector2 uvC;
} e3d_TriangleToRender;

typedef struct {
  e3d_Vector2 a;
  e3d_Vector2 b;
  e3d_Vector2 c;
} e3d_Triangle2D;

typedef struct {
  e3d_Vector3 a;
  e3d_Vector3 b;
  e3d_Vector3 c;
} e3d_Triangle3D;

typedef struct {
  e3d_Triangle3D TriangleOnScreen;
  e3d_Triangle2D UV;
  int32_t LightDistances[3];
  const e3d_Material *mat;
} e3d_TriangleInScene;

typedef struct {
  e3d_Vector3 point;
  uint16_t color;
} e3d_Point3D;

typedef struct {
  e3d_Vector2 point;
  uint16_t color;
} e3d_PointInScene;

typedef struct {
  e3d_Vector3 start;
  e3d_Vector3 end;
  uint16_t color;
} e3d_Line3D;

typedef struct {
  e3d_Vector2 start;
  e3d_Vector2 end;
  uint16_t color;
} e3d_LineInScene;

typedef enum {
  POINT = 0,
  LINE = 1,
  TRIANGLE = 2,
} e3d_PrimType;

typedef union {
  e3d_PointInScene point;
  e3d_LineInScene line;
  e3d_TriangleInScene triangle;
} e3d_PrimitivePayload;

typedef struct {
  // The center is calculated when the primitive is added to the scene and is
  // used only as the painter-sort key. Rendering uses the payload coordinates.
  e3d_Vector3 pos;
  e3d_PrimType type;
  e3d_PrimitivePayload payload;
} e3d_Primitive;

#endif
