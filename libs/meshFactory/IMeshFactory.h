#ifndef IMESHFACTORY_h
#define IMESHFACTORY_h

#include "IStorage.h"
#include "material.h"
#include "mesh.h"
#include <stdint.h>

typedef struct {
  void (*init_mesh_factory)(const IStorage *storage);
  Mesh *(*create_mesh)(Material *mat, uint8_t meshIndex);
} IMeshFactory;

#endif