
#ifndef IMATERIALFACTORY_h
#define IMATERIALFACTORY_h

#include "IStorage.h"
#include "material.h"
#include <stdint.h>

typedef struct {
  void (*init_material_factory)(const e3d_IStorage *storage);
  e3d_Material *(*create_diffuse_mat)(uint16_t color, float roughness,
                                  float metallic);
  e3d_Material *(*create_textured_mat)(uint8_t imageIndex, float roughness,
                                   float metallic, bool transparent);
} e3d_IMaterialFactory;

#endif