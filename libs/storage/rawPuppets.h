#ifndef RAWPUPPETS_h
#define RAWPUPPETS_h

#include "gfx.h"
#include "sprites.h"
#include "stdio.h"
#include "vectors.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  const int x;
  const int y;
  const float angle;
  const int startFrameNum;
} e3d_RawFrame;

typedef struct {
  const e3d_RawFrame *frames;
  const uint16_t framesNum;
} e3d_RawAnimation;

typedef struct e3d_RawPuppetBone e3d_RawPuppetBone;

typedef struct e3d_RawPuppetBone {
  const char *label;
  const int16_t x;
  const int16_t y;
  const float angle;
  const uint8_t spriteIndex;
  const float baseSpriteAngle;
  const e3d_RawPuppetBone *childPuppetBonesLayer1;
  const uint8_t childPuppetBonesNumLayer1;
  const e3d_RawPuppetBone *childPuppetBonesLayer2;
  const uint8_t childPuppetBonesNumLayer2;
} e3d_RawPuppetBone;

typedef struct {
  const e3d_RawPuppetBone *rawBone;
  const e3d_RawAnimation *rawAnimation;
} e3d_RawBoneAnimationPair;

typedef struct {
  const char *label;
  const int16_t x;
  const int16_t y;
  const float angle;
  const e3d_RawPuppetBone *puppetBones;
  const uint8_t puppetBonesNum;
  const e3d_RawBoneAnimationPair *boneAnimationPairs;
  const uint8_t boneAnimationPairsNum;
} e3d_RawPuppet;

#endif