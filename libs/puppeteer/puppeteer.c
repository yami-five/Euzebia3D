#include "IPuppeteer.h"
#include "IPainter.h"
#include "IStorage.h"
#include "IPuppetFactory.h"
#include "puppetCalc.h"
#include "puppeteer.h"
#include "puppetFactory.h"
#include "puppet.h"
#include "string.h"
#include "../storage/rawPuppets.h"

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "hardware/interp.h"
interp_config blend_cfg;
interp_config alpha_cfg;
#endif

static const IPainter *_painter;
static const IPuppetFactory *_puppetFactory;

void init_puppeteer(const IStorage *storage, const IPainter *painter)
{
    _puppetFactory = get_puppetFactory();
    _puppetFactory->init_puppet_factory(storage);
    _painter = painter;

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
    blend_cfg = interp_default_config();
    interp_config_set_blend(&blend_cfg, true);
    interp_set_config(interp0, 0, &blend_cfg);

    alpha_cfg = interp_default_config();
    interp_config_set_mask(&alpha_cfg, 0, 7);
    interp_config_set_signed(&alpha_cfg, true);
    interp_set_config(interp0, 1, &alpha_cfg);
#endif
}

Puppet *create_puppet(uint8_t puppetIndex)
{
    _puppetFactory->create(puppetIndex);
}

void draw_PuppetBone(PuppetBone *PuppetBone, int *parentWorldMatrix)
{
    for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer2; i++)
    {
        draw_PuppetBone(&PuppetBone->childPuppetBonesLayer2[i], PuppetBone->worldMatrix);
    }
    if (PuppetBone->sprite != NULL)
    {
        if (PuppetBone->sprite->width != PuppetBone->sprite->height)
            return;

        uint8_t spriteWidthHalved = PuppetBone->sprite->width >> 1;
        uint8_t spriteHeightHalved = PuppetBone->sprite->height >> 1;
        int16_t startX = PuppetBone->worldMatrix[2] >> SHIFT_FACTOR;
        int16_t startY = PuppetBone->worldMatrix[5] >> SHIFT_FACTOR;
        int16_t parentX = parentWorldMatrix[2] >> SHIFT_FACTOR;
        int16_t parentY = parentWorldMatrix[5] >> SHIFT_FACTOR;
        int32_t angle = 0;
        if (PuppetBone->sprite->canRotate)
        {
            angle = fast_atan2(startY - parentY, startX - parentX) + PuppetBone->baseSpriteAngle;
            angle = radian_to_index(angle);
        }
        startX += ((parentX - startX) >> 1) - spriteWidthHalved;
        startY += ((parentY - startY) >> 1) - spriteHeightHalved;
        _painter->draw_sprite(PuppetBone->sprite, startX, startY, angle, 1);
    }
    for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer1; i++)
    {
        draw_PuppetBone(&PuppetBone->childPuppetBonesLayer1[i], PuppetBone->worldMatrix);
    }
}

void draw_puppet(Puppet *puppet)
{
    for (uint8_t i = 0; i < puppet->puppetBonesNum; i++)
    {
        draw_PuppetBone(&puppet->puppetBones[i], puppet->worldMatrix);
    }
}

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
static int32_t anim_lerp_i32(int32_t a, int32_t b, uint8_t alpha)
{
    interp_set_base(interp0, 0, (uint32_t)a);
    interp_set_base(interp0, 1, (uint32_t)b);
    interp_set_accumulator(interp0, 1, alpha);
    return (int32_t)interp_peek_lane_result(interp0, 1);
}
#else
static int32_t anim_lerp_i32(int32_t a, int32_t b, uint8_t alpha)
{
    return a + (((b - a) * alpha) >> 8);
}
#endif

void perform(Puppet *puppet, uint32_t t)
{
    if (puppet->boneTimelinePairsNum <= 0 || puppet->puppetBonesNum <= 0)
        return;
    if (puppet->animationStartFrame < 0)
        puppet->animationStartFrame = t;
    int localAnimFrame = t - puppet->animationStartFrame;
    for (uint8_t i = 0; i < puppet->boneTimelinePairsNum; i++)
    {
        if (
            localAnimFrame < puppet->boneTimelinePairs[i].boneTimeline->keyFrames[0].startFrameNum ||
            localAnimFrame > puppet->boneTimelinePairs[i].boneTimeline->keyFrames[puppet->boneTimelinePairs[i].boneTimeline->keyFramesNum - 1].startFrameNum)
            continue;
        PuppetBoneTimelinePair pair = puppet->boneTimelinePairs[i];
        for (uint8_t j = 0; j < pair.boneTimeline->keyFramesNum - 1; j++)
        {
            KeyFrame *keyFrame = &pair.boneTimeline->keyFrames[j];
            KeyFrame *nextKeyFrame = &pair.boneTimeline->keyFrames[j + 1];

            if (localAnimFrame >= keyFrame->startFrameNum &&
                localAnimFrame < nextKeyFrame->startFrameNum)
            {
                uint32_t span = nextKeyFrame->startFrameNum - keyFrame->startFrameNum;
                uint32_t frameInSpan = localAnimFrame - keyFrame->startFrameNum;
                uint8_t alpha = (uint8_t)((frameInSpan << 8) / span);

                pair.bone->x = (int16_t)anim_lerp_i32(keyFrame->x, nextKeyFrame->x, alpha);
                pair.bone->y = (int16_t)anim_lerp_i32(keyFrame->y, nextKeyFrame->y, alpha);

                int32_t angleFixed = anim_lerp_i32(
                    float_to_fixed(keyFrame->angle),
                    float_to_fixed(nextKeyFrame->angle),
                    alpha
                );
                pair.bone->angle = (float)angleFixed / (float)SCALE_FACTOR;
                break;
            }
            else if (localAnimFrame == nextKeyFrame->startFrameNum)
            {
                pair.bone->x=nextKeyFrame->x;
                pair.bone->y=nextKeyFrame->y;
                pair.bone->angle=nextKeyFrame->angle;
                break;
            }
        }
    }
    update_world_matrices(puppet);
    draw_puppet(puppet);
}

static IPuppeteer puppet = {
    .init_puppeteer = init_puppeteer,
    .create_puppet = create_puppet,
    .perform = perform,
};

const IPuppeteer *get_puppeteer(void)
{
    return &puppet;
}
