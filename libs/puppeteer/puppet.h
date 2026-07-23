#ifndef PUPPET_h
#define PUPPET_h

#include "../storage/gfx.h"
#include "../storage/rawPuppets.h"
#include "../storage/sprites.h"
#include "stdio.h"
#include "vectors.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  int x;
  int y;
  float angle;
  int startFrameNum;
} e3d_KeyFrame;

typedef struct {
  e3d_KeyFrame *keyFrames;
  uint16_t keyFramesNum;
} e3d_PuppetBoneAnimTimeline;

typedef struct e3d_PuppetBone e3d_PuppetBone;

typedef struct e3d_PuppetBone {
  const char *label;
  int16_t x;
  int16_t y;
  float angle;
  const e3d_Sprite *sprite;
  int16_t baseSpriteAngle;
  int worldMatrix[9];
  int localMatrix[9];
  e3d_PuppetBone *childPuppetBonesLayer1;
  uint8_t childPuppetBonesNumLayer1;
  e3d_PuppetBone *childPuppetBonesLayer2;
  uint8_t childPuppetBonesNumLayer2;
} e3d_PuppetBone;

typedef struct {
  e3d_PuppetBoneAnimTimeline *boneTimeline;
  e3d_PuppetBone *bone;
} e3d_PuppetBoneTimelinePair;

typedef struct {
  const char *label;
  int16_t x;
  int16_t y;
  float angle;
  int worldMatrix[9];
  int localMatrix[9];
  e3d_PuppetBone *puppetBones;
  uint8_t puppetBonesNum;
  e3d_PuppetBoneTimelinePair *boneTimelinePairs;
  uint8_t boneTimelinePairsNum;
  int32_t animationStartFrame;
} e3d_Puppet;

#endif
