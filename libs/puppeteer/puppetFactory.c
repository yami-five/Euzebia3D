#include "puppetFactory.h"
#include "../storage/rawPuppets.h"
#include "IPuppetFactory.h"
#include "puppet.h"
#include "puppetCalc.h"
#include "string.h"

static const e3d_IStorage *_storage;

void init_puppet_factory(const e3d_IStorage *storage) { _storage = storage; }

e3d_PuppetBone *create_PuppetBones(const e3d_RawPuppetBone *rawPuppetBones,
                               const uint8_t puppetBonesNum,
                               int *parentWorldMatrix) {
  e3d_PuppetBone *newPuppetBones =
      (e3d_PuppetBone *)malloc(sizeof(e3d_PuppetBone) * puppetBonesNum);
  for (uint8_t i = 0; i < puppetBonesNum; i++) {
    newPuppetBones[i].label = rawPuppetBones[i].label;
    newPuppetBones[i].x = rawPuppetBones[i].x;
    newPuppetBones[i].y = rawPuppetBones[i].y;
    newPuppetBones[i].angle = rawPuppetBones[i].angle;
    newPuppetBones[i].sprite =
        _storage->get_sprite(rawPuppetBones[i].spriteIndex);
    newPuppetBones[i].baseSpriteAngle =
        float_to_fixed(rawPuppetBones[i].baseSpriteAngle);
    make_local_matrix(&newPuppetBones[i]);
    make_world_matrix(&newPuppetBones[i], parentWorldMatrix);
    newPuppetBones[i].childPuppetBonesNumLayer1 =
        rawPuppetBones[i].childPuppetBonesNumLayer1;
    newPuppetBones[i].childPuppetBonesNumLayer2 =
        rawPuppetBones[i].childPuppetBonesNumLayer2;
    newPuppetBones[i].childPuppetBonesLayer1 = NULL;
    newPuppetBones[i].childPuppetBonesLayer2 = NULL;
    if (rawPuppetBones[i].childPuppetBonesNumLayer1 != 0)
      newPuppetBones[i].childPuppetBonesLayer1 =
          create_PuppetBones(rawPuppetBones[i].childPuppetBonesLayer1,
                             rawPuppetBones[i].childPuppetBonesNumLayer1,
                             newPuppetBones[i].worldMatrix);
    if (rawPuppetBones[i].childPuppetBonesNumLayer2 != 0)
      newPuppetBones[i].childPuppetBonesLayer2 =
          create_PuppetBones(rawPuppetBones[i].childPuppetBonesLayer2,
                             rawPuppetBones[i].childPuppetBonesNumLayer2,
                             newPuppetBones[i].worldMatrix);
  }
  return newPuppetBones;
}

static e3d_PuppetBone *find_PuppetBone_by_label(e3d_PuppetBone *puppetBones,
                                            uint8_t puppetBonesNum,
                                            const char *label) {
  if (puppetBones == NULL || label == NULL)
    return NULL;

  for (uint8_t i = 0; i < puppetBonesNum; i++) {
    e3d_PuppetBone *found;

    if (puppetBones[i].label == label)
      return &puppetBones[i];

    found = find_PuppetBone_by_label(puppetBones[i].childPuppetBonesLayer1,
                                     puppetBones[i].childPuppetBonesNumLayer1,
                                     label);
    if (found != NULL)
      return found;

    found = find_PuppetBone_by_label(puppetBones[i].childPuppetBonesLayer2,
                                     puppetBones[i].childPuppetBonesNumLayer2,
                                     label);
    if (found != NULL)
      return found;
  }

  return NULL;
}

e3d_PuppetBoneTimelinePair *
create_PuppetBoneTimelinePair(const e3d_RawPuppet *rawPuppet,
                              e3d_PuppetBone *puppetBones) {
  e3d_PuppetBoneTimelinePair *newPairs = (e3d_PuppetBoneTimelinePair *)malloc(
      sizeof(e3d_PuppetBoneTimelinePair) * rawPuppet->boneAnimationPairsNum);
  for (uint8_t i = 0; i < rawPuppet->boneAnimationPairsNum; i++) {
    const e3d_RawBoneAnimationPair *rawPair = &rawPuppet->boneAnimationPairs[i];
    e3d_PuppetBone *bone = NULL;
    e3d_PuppetBoneAnimTimeline *newTimeline = NULL;

    newPairs[i].bone = NULL;
    newPairs[i].boneTimeline = NULL;

    if (rawPair->rawBone != NULL)
      bone = find_PuppetBone_by_label(puppetBones, rawPuppet->puppetBonesNum,
                                      rawPair->rawBone->label);

    if (bone != NULL && rawPair->rawAnimation != NULL) {
      newTimeline =
          (e3d_PuppetBoneAnimTimeline *)malloc(sizeof(e3d_PuppetBoneAnimTimeline));
      if (newTimeline != NULL) {
        newTimeline->keyFramesNum = rawPair->rawAnimation->framesNum;
        newTimeline->keyFrames =
            (e3d_KeyFrame *)malloc(sizeof(e3d_KeyFrame) * newTimeline->keyFramesNum);
        if (newTimeline->keyFrames != NULL) {
          for (uint16_t k = 0; k < rawPair->rawAnimation->framesNum; k++) {
            newTimeline->keyFrames[k].x = rawPair->rawAnimation->frames[k].x;
            newTimeline->keyFrames[k].y = rawPair->rawAnimation->frames[k].y;
            newTimeline->keyFrames[k].angle =
                rawPair->rawAnimation->frames[k].angle;
            newTimeline->keyFrames[k].startFrameNum =
                rawPair->rawAnimation->frames[k].startFrameNum;
          }
          newPairs[i].boneTimeline = newTimeline;
          newPairs[i].bone = bone;
        } else {
          free(newTimeline);
        }
      }
    }
  }
  return newPairs;
}

e3d_Puppet *create(uint8_t puppetIndex) {
  e3d_Puppet *newPuppet = (e3d_Puppet *)malloc(sizeof(e3d_Puppet));
  const e3d_RawPuppet *rawPuppet = _storage->get_raw_puppet(puppetIndex);
  newPuppet->x = rawPuppet->x;
  newPuppet->y = rawPuppet->y;
  newPuppet->angle = rawPuppet->angle;
  newPuppet->puppetBonesNum = rawPuppet->puppetBonesNum;
  newPuppet->boneTimelinePairsNum = rawPuppet->boneAnimationPairsNum;
  newPuppet->animationStartFrame = -1;
  int32_t angleFixed = float_to_fixed(newPuppet->angle);
  int16_t sin = fast_sin(angleFixed);
  int16_t cos = fast_cos(angleFixed);
  newPuppet->localMatrix[0] = cos;
  newPuppet->localMatrix[1] = -sin;
  newPuppet->localMatrix[2] = newPuppet->x;
  newPuppet->localMatrix[3] = sin;
  newPuppet->localMatrix[4] = cos;
  newPuppet->localMatrix[5] = newPuppet->y;
  newPuppet->localMatrix[6] = newPuppet->localMatrix[7] = 0;
  newPuppet->localMatrix[8] = SCALE_FACTOR;
  memcpy(newPuppet->worldMatrix, newPuppet->localMatrix,
         sizeof(newPuppet->localMatrix));
  if (rawPuppet->puppetBonesNum != 0)
    newPuppet->puppetBones =
        create_PuppetBones(rawPuppet->puppetBones, rawPuppet->puppetBonesNum,
                           newPuppet->worldMatrix);
  if (rawPuppet->boneAnimationPairsNum != 0)
    newPuppet->boneTimelinePairs =
        create_PuppetBoneTimelinePair(rawPuppet, newPuppet->puppetBones);
  return newPuppet;
}

static e3d_IPuppetFactory puppet = {
    .init_puppet_factory = init_puppet_factory,
    .create = create,
};

const e3d_IPuppetFactory *get_puppetFactory(void) { return &puppet; }
