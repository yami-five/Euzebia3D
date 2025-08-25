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
} Triangle2D;

typedef struct
{
    Vector3 a;
    Vector3 b;
    Vector3 c;
} Triangle3D;

const IRenderer *get_renderer(void);

#endif