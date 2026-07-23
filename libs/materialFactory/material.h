#ifndef MATERIAL_h
#define MATERIAL_h

#include "../storage/gfx.h"
#include "fpa.h"
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint16_t diffuse;
  const uint16_t *texture;
  int textureSize;
  int textureWidth;
  int textureHeight;
  bool transparent;
  uint8_t roughness;
  uint8_t metallic;
} e3d_Material;

void free_material(e3d_Material *mat);

#endif
