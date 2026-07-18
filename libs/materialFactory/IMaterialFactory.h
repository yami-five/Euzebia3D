
#ifndef IMATERIALFACTORY_h
#define IMATERIALFACTORY_h

#include "IStorage.h"
#include "material.h"
#include <stdint.h>

typedef struct {
  void (*init_material_factory)(const IStorage *storage);
  Material *(*create_diffuse_mat)(uint16_t color, float roughness,
                                  float metallic);
  Material *(*create_textured_mat)(uint8_t imageIndex, float roughness,
                                   float metallic, bool transparent);
} IMaterialFactory;

#endif