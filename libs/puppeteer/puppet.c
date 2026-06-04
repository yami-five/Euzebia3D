#include "puppet.h"
#include <string.h>
#include "IStorage.h"

// void move_puppet(Puppet *puppet, int16_t newX, int16_t newY)
// {
//     puppet->x += newX;
//     puppet->y += newY;
//     update_world_matrices(puppet);
// }

// void transform_PuppetBone(PuppetBone *PuppetBone, int16_t x, int16_t y, float angle)
// {
//     if (!PuppetBone)
//         return;
//     PuppetBone->x += x;
//     PuppetBone->y += y;
//     PuppetBone->angle += angle;
// }

// void animate_PuppetBones(PuppetBoneSkeletalAnimation *PuppetBoneSkeletalAnimations, uint8_t animationsNum, uint32_t frameNum, bool invert)
// {
//     for (uint8_t i = 0; i < animationsNum; i++)
//     {
//         const SkeletalAnimation *skeletalAnimation = PuppetBoneSkeletalAnimations[i].skeletalAnimation;
//         uint32_t localFrameNum;

//         if (skeletalAnimation == NULL || skeletalAnimation->bakedFrames == NULL || skeletalAnimation->bakedFramesNum == 0)
//             continue;
//         if (frameNum < skeletalAnimation->timelineStart)
//             continue;

//         localFrameNum = frameNum - skeletalAnimation->timelineStart;
//         while (localFrameNum >= skeletalAnimation->bakedFramesNum)
//             localFrameNum -= skeletalAnimation->bakedFramesNum;
//         if (invert)
//             localFrameNum = skeletalAnimation->bakedFramesNum - 1u - localFrameNum;

//         const SkeletalAnimationFrame *frame = &skeletalAnimation->bakedFrames[localFrameNum];
//         if (invert)
//             transform_PuppetBone(PuppetBoneSkeletalAnimations[i].PuppetBone, -frame->x, -frame->y, -frame->angle);
//         else
//             transform_PuppetBone(PuppetBoneSkeletalAnimations[i].PuppetBone, frame->x, frame->y, frame->angle);
//     }
// }

// void change_sprite(PuppetBone *PuppetBone, const Sprite *newSprite)
// {
//     PuppetBone->sprite = newSprite;
// }
