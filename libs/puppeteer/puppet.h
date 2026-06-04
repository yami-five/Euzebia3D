#ifndef PUPPET_h
#define PUPPET_h

#include "stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include "../storage/gfx.h"
#include "vectors.h"
#include "../storage/sprites.h"
#include "../storage/rawPuppets.h"

typedef struct
{
    int x;
    int y;
    float angle;
    int startFrameNum;
} KeyFrame;

typedef struct
{
    KeyFrame* keyFrames;
    uint16_t keyFramesNum;
} PuppetBoneAnimTimeline;

typedef struct PuppetBone PuppetBone;

typedef struct PuppetBone
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    const Sprite *sprite;
    int16_t baseSpriteAngle;
    int worldMatrix[9];
    int localMatrix[9];
    PuppetBone *childPuppetBonesLayer1;
    uint8_t childPuppetBonesNumLayer1;
    PuppetBone *childPuppetBonesLayer2;
    uint8_t childPuppetBonesNumLayer2;
} PuppetBone;

typedef struct 
{
    PuppetBoneAnimTimeline* boneTimeline;
    PuppetBone* bone;
}PuppetBoneTimelinePair;

typedef struct
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    int worldMatrix[9];
    int localMatrix[9];
    PuppetBone *puppetBones;
    uint8_t puppetBonesNum;
    PuppetBoneTimelinePair *boneTimelinePairs;
    uint8_t boneTimelinePairsNum;
    int32_t animationStartFrame;
} Puppet;

#endif
