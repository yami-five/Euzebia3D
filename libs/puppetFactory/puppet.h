#ifndef PUPPET_h
#define PUPPET_h

#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../storage/gfx.h"
#include "vectors.h"
#include "../storage/sprites.h"
#include "../storage/rawPuppets.h"
#include "animation.h"

typedef struct Bone Bone;

typedef struct Bone
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    const Sprite *sprite;
    int16_t baseSpriteAngle;
    int worldMatrix[9];
    int localMatrix[9];
    Bone *childBonesLayer1;
    uint8_t childBonesNumLayer1;
    Bone *childBonesLayer2;
    uint8_t childBonesNumLayer2;
} Bone;

typedef struct
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    int worldMatrix[9];
    int localMatrix[9];
    Bone *bones;
    uint8_t bonesNum;
} Puppet;

typedef struct
{
    float x;
    float y;
    float angle;
} BoneTransform;

void make_local_matrix(Bone *bone);
void make_world_matrix(Bone *bone, int *parentWorldMatrix);
void update_world_matrices(Puppet *puppet);
void move_puppet(Puppet *puppet, int16_t newX, int16_t newY);
Bone *get_bone_by_name(Bone *bone, const char *boneLabel);
void transform_bone(Bone *bone, int16_t x, int16_t y, float angle);
const AnimationClip *get_animation_clip_by_label(const char *label);
void animate_clip(Puppet *puppet, const AnimationClip *clip, uint32_t frameNum, bool invert);
void change_sprite(Bone *bone, const Sprite *newSprite);

#endif
