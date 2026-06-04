#ifndef RAWPUPPETS_h
#define RAWPUPPETS_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "gfx.h"
#include "vectors.h"
#include "sprites.h"

typedef struct
{
    const int x;
    const int y;
    const float angle;
    const int startFrameNum;
} RawFrame;

typedef struct
{
    const RawFrame *frames;
    const uint16_t framesNum;
} RawAnimation;

typedef struct RawPuppetBone RawPuppetBone;

typedef struct RawPuppetBone
{
    const char *label;
    const int16_t x;
    const int16_t y;
    const float angle;
    const uint8_t spriteIndex;
    const float baseSpriteAngle;
    const RawPuppetBone *childPuppetBonesLayer1;
    const uint8_t childPuppetBonesNumLayer1;
    const RawPuppetBone *childPuppetBonesLayer2;
    const uint8_t childPuppetBonesNumLayer2;
} RawPuppetBone;

typedef struct
{
    const RawPuppetBone *rawBone;
    const RawAnimation *rawAnimation;
} RawBoneAnimationPair;

typedef struct
{
    const char *label;
    const int16_t x;
    const int16_t y;
    const float angle;
    const RawPuppetBone *puppetBones;
    const uint8_t puppetBonesNum;
    const RawBoneAnimationPair *boneAnimationPairs;
    const uint8_t boneAnimationPairsNum;
} RawPuppet;

#endif