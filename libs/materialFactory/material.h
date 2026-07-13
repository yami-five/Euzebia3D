#ifndef MATERIAL_h
#define MATERIAL_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fpa.h"
#include "../storage/gfx.h"

typedef struct
{
    uint16_t diffuse;
    const uint16_t *texture;
    int textureSize;
    int textureWidth;
    int textureHeight;
    bool transparent;
    uint8_t roughness;
    uint8_t metallic;
} Material;

void free_material(Material *mat);

#endif
