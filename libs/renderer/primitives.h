#ifndef PRIMITIVES_h
#define PRIMITIVES_h

#include "material.h"
#include "vectors.h"
#include <stdint.h>

typedef struct
{
    Vector4 a;
    Vector4 b;
    Vector4 c;
    Vector2 uvA;
    Vector2 uvB;
    Vector2 uvC;
} TriangleToRender;

typedef struct {
  Vector2 a;
  Vector2 b;
  Vector2 c;
} Triangle2D;

typedef struct {
  Vector3 a;
  Vector3 b;
  Vector3 c;
} Triangle3D;

typedef struct {
  Triangle3D TriangleOnScreen;
  Triangle2D UV;
  int32_t LightDistances[3];
  Material *mat;
} TriangleInScene;

typedef struct {
  Vector3 point;
  uint16_t color;
} Point3D;

typedef struct {
  Vector2 point;
  uint16_t color;
} PointInScene;

typedef struct {
  Vector3 start;
  Vector3 end;
  uint16_t color;
} Line3D;

typedef struct {
  Vector2 start;
  Vector2 end;
  uint16_t color;
} LineInScene;

typedef enum {
  POINT = 0,
  LINE = 1,
  TRIANGLE = 2,
} PrimType;

typedef union {
  PointInScene point;
  LineInScene line;
  TriangleInScene triangle;
} PrimitivePayload;

typedef struct {
  // The center is calculated when the primitive is added to the scene and is
  // used only as the painter-sort key. Rendering uses the payload coordinates.
  Vector3 pos;
  PrimType type;
  PrimitivePayload payload;
} Primitive;

#endif
