#ifndef RAWPUPPETS_h
#define RAWPUPPETS_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "gfx.h"
#include "vectors.h"
#include "sprites.h"

typedef struct RawBone RawBone;

typedef struct RawBone
{
    const char *label;
    const int16_t x;
    const int16_t y;
    const float angle;
    const uint8_t spriteIndex;
    const float baseSpriteAngle;
    const RawBone *childBonesLayer1;
    const uint8_t childBonesNumLayer1;
    const RawBone *childBonesLayer2;
    const uint8_t childBonesNumLayer2;
} RawBone;

typedef struct
{
    const char* label;
    const int16_t x;
    const int16_t y;
    const float angle;
    const RawBone *bones;
    const uint8_t bonesNum;
} RawPuppet;

const RawPuppet* get_raw_puppet(uint8_t puppetIndex);

#endif