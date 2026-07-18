#include "meshFactory.h"
#include "IMeshFactory.h"
#include "IStorage.h"
#include "fpa.h"
#include "mesh.h"
#include <stdlib.h>

static const IStorage *_storage = NULL;

void init_mesh_factory(const IStorage *storage) { _storage = storage; }

Mesh *create_mesh(Material *mat, uint8_t meshIndex) {
  if (_storage == NULL || mat == NULL) {
    free(mat);
    return NULL;
  }

  const Model *obj = _storage->get_model(meshIndex);
  if (obj == NULL) {
    free(mat);
    return NULL;
  }

  Mesh *mesh = (Mesh *)calloc(1, sizeof(Mesh));
  if (mesh == NULL) {
    free(mat);
    return NULL;
  }

  size_t vertexValues = (size_t)obj->verticesCounter * 3u;
  size_t faceValues = (size_t)obj->facesCounter * 3u;
  size_t textureCoordValues = (size_t)obj->textureCoordsCounter * 2u;
  size_t normalValues = (size_t)obj->vnCounter * 3u;

  mesh->verticesCounter = obj->verticesCounter;
  mesh->facesCounter = obj->facesCounter;
  mesh->textureCoordsCounter = obj->textureCoordsCounter;
  mesh->vnCounter = obj->vnCounter;
  mesh->normalsCounter = (uint16_t)faceValues;
  mesh->mat = mat;
  mesh->vertices = (int32_t *)malloc(sizeof(int32_t) * vertexValues);
  mesh->faces = (uint16_t *)malloc(sizeof(uint16_t) * faceValues);
  mesh->textureCoords = (int32_t *)malloc(sizeof(int32_t) * textureCoordValues);
  mesh->uv = (uint16_t *)malloc(sizeof(uint16_t) * faceValues);
  mesh->vn = (int32_t *)malloc(sizeof(int32_t) * normalValues);
  mesh->normals = (uint16_t *)malloc(sizeof(uint16_t) * faceValues);

  if ((vertexValues != 0 && mesh->vertices == NULL) ||
      (faceValues != 0 && mesh->faces == NULL) ||
      (textureCoordValues != 0 && mesh->textureCoords == NULL) ||
      (faceValues != 0 && mesh->uv == NULL) ||
      (normalValues != 0 && mesh->vn == NULL) ||
      (faceValues != 0 && mesh->normals == NULL)) {
    free_model(mesh);
    return NULL;
  }

  for (size_t i = 0; i < faceValues; i++) {
    mesh->faces[i] = obj->faces[i];
    mesh->uv[i] = obj->uv[i];
    mesh->normals[i] = obj->normals[i];
  }

  for (size_t i = 0; i < vertexValues; i++) {
    mesh->vertices[i] = float_to_fixed(obj->vertices[i]);
  }

  for (size_t i = 0; i < normalValues; i++) {
    mesh->vn[i] = float_to_fixed(obj->vn[i]);
  }

  for (size_t i = 0; i < textureCoordValues; i++) {
    mesh->textureCoords[i] = float_to_fixed(obj->textureCoords[i]);
  }

  // free((void *)obj);
  return mesh;
}

static IMeshFactory meshFactory = {
    .init_mesh_factory = init_mesh_factory,
    .create_mesh = create_mesh,
};

const IMeshFactory *get_meshFactory(void) { return &meshFactory; }
