#ifndef MESH_h
#define MESH_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "fpa.h"
#include "../storage/gfx.h"

typedef struct
{
    uint16_t diffuse;
    const uint16_t *texture;
    int textureSize;
    uint8_t isSkyBox;
} Material;

typedef struct
{
    int32_t w;
    int32_t x;
    int32_t y;
    int32_t z;
} TransformVector;

typedef struct
{
    uint8_t transformType;
    TransformVector *transformVector;
} TransformInfo;

typedef struct
{
    uint16_t verticesCounter;
    uint16_t facesCounter;
    int32_t *vertices;
    uint16_t *faces;
    int32_t *textureCoords;
    uint16_t *uv;
    uint16_t normalsCounter;
    uint16_t *normals;
    int32_t *vn;
    uint16_t vnCounter;
    Material *mat;
    TransformInfo *transformations;
    uint32_t transformationsNum;
} Mesh;

TransformInfo *add_transformation(TransformInfo *currentTransformations, uint32_t *currentTransformationsNum, float w, float x, float y, float z, uint8_t transformationType);
void modify_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex);
void free_model(Mesh *mesh);

#endif