#ifndef GFX_H
#define GFX_H

#include "stdint.h"

typedef struct
{
    const uint16_t *image;
    const uint32_t size;
    const uint16_t width;
    const uint16_t heigth; 
} Image;

typedef struct
{
    const uint16_t verticesCounter;
    const uint16_t facesCounter;
    const uint16_t textureCoordsCounter;
    const float *vertices;
    const uint16_t *faces;
    const float *textureCoords;
    const uint16_t *uv;
    const float *vn;
    const uint16_t vnCounter;
    const uint16_t *normals;
    const uint16_t nn;
} Model;

#endif