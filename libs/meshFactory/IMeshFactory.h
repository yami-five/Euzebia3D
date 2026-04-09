#ifndef IMESHFACTORY_h
#define IMESHFACTORY_h

#include <stdint.h>
#include "IStorage.h"
#include "mesh.h"

typedef struct
{
    void (*init_mesh_factory)(volatile const IStorage * storage);
    Mesh* (*create_colored_mesh)(uint16_t color, uint8_t meshIndex);
    Mesh* (*create_textured_mesh)(uint8_t imageIndex, uint8_t meshIndex);
    Mesh* (*create_colored_skybox)(uint16_t color);
    Mesh* (*create_textured_skybox)(uint8_t imageIndex);
} IMeshFactory;

#endif