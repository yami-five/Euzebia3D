#ifndef RENDERER_h
#define RENDERER_h

#include "IRenderer.h"
#include "fpa.h"
#include "camera.h"
#include "light.h"
#include "mesh.h"
#include "string.h"

typedef struct
{
    Vector3 a;
    Vector3 b;
    Vector3 c;
    Vector2 uvA;
    Vector2 uvB;
    Vector2 uvC;
} TriangleToRender;

typedef struct
{
    Vector2 a;
    Vector2 b;
    Vector2 c;
} Triangle2D;

typedef struct
{
    Vector3 a;
    Vector3 b;
    Vector3 c;
} Triangle3D;

typedef struct
{
    Triangle3D TriangleOnScreen;
    Triangle2D UV;
    int32_t LightDistances[3];
    Material *mat;
} TriangleInScene;

const IRenderer *get_renderer(void);
void renderer_set_scale(uint8_t scale);

#endif
