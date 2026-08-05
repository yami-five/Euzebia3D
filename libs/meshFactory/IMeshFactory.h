#ifndef IMESHFACTORY_h
#define IMESHFACTORY_h

#include "IStorage.h"
#include "material.h"
#include "mesh.h"
#include <stdint.h>

typedef struct {
  void (*init_mesh_factory)(const e3d_IStorage *storage);
  e3d_Mesh *(*create_mesh)(const e3d_Material *mat, uint8_t meshIndex);
} e3d_IMeshFactory;

#endif
