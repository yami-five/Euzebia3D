#ifndef MESH_h
#define MESH_h

#include "../storage/gfx.h"
#include "fpa.h"
#include "material.h"
#include "stdio.h"
#include "transformations.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint16_t verticesCounter;
  uint16_t facesCounter;
  int32_t *vertices;
  uint16_t *faces;
  int32_t *textureCoords;
  uint16_t textureCoordsCounter;
  uint16_t *uv;
  uint16_t normalsCounter;
  uint16_t *normals;
  int32_t *vn;
  uint16_t vnCounter;
  e3d_Material *mat;
  e3d_TransformInfo *transformations;
  uint32_t transformationsNum;
} e3d_Mesh;

e3d_TransformInfo *add_transformation(e3d_TransformInfo *currentTransformations,
                                  uint32_t *currentTransformationsNum, float w,
                                  float x, float y, float z,
                                  e3d_ModelTransformType transformationType);
void modify_mesh_transformation(e3d_TransformInfo *currentTransformations, float w,
                                float x, float y, float z,
                                uint32_t transformationIndex);
void free_model(e3d_Mesh *mesh);

#endif
