#include "IPainter.h"
#include "IPuppeteer.h"
#include "IStorage.h"
#include "puppeteer.h"
#include "puppetFactory.h"
#include "puppet.h"
#include "string.h"
#include "../storage/rawPuppets.h"

static const IPainter *_painter;
static const IPuppetFactory *_puppetFactory;

void init_puppeteer(const IStorage *storage, const IPainter *painter)
{
    _puppetFactory = get_puppetFactory;
    _puppetFactory->init_puppet_factory(storage);
    _painter = painter;
}

Puppet *create_puppet(uint8_t puppetIndex)
{
    _puppetFactory->create_puppet(puppetIndex);
}

// void draw_PuppetBone(PuppetBone *PuppetBone, int *parentWorldMatrix)
// {
//     for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer2; i++)
//     {
//         draw_PuppetBone(&PuppetBone->childPuppetBonesLayer2[i], PuppetBone->worldMatrix);
//     }
//     if (PuppetBone->sprite != NULL)
//     {
//         if (PuppetBone->sprite->width != PuppetBone->sprite->height)
//             return;

//         uint8_t spriteWidthHalved = PuppetBone->sprite->width >> 1;
//         uint8_t spriteHeightHalved = PuppetBone->sprite->height >> 1;
//         int16_t startX = PuppetBone->worldMatrix[2] >> SHIFT_FACTOR;
//         int16_t startY = PuppetBone->worldMatrix[5] >> SHIFT_FACTOR;
//         int16_t parentX = parentWorldMatrix[2] >> SHIFT_FACTOR;
//         int16_t parentY = parentWorldMatrix[5] >> SHIFT_FACTOR;
//         int32_t angle = 0;
//         if (PuppetBone->sprite->canRotate)
//         {
//             angle = fast_atan2(startY - parentY, startX - parentX) + PuppetBone->baseSpriteAngle;
//             angle = radian_to_index(angle);
//         }
//         startX += ((parentX - startX) >> 1) - spriteWidthHalved;
//         startY += ((parentY - startY) >> 1) - spriteHeightHalved;
//         draw_sprite(PuppetBone->sprite, startX, startY, angle, 1);
//     }
//     // draw_sprite(PuppetBone->sprite, x, y, 0.0f);
//     for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer1; i++)
//     {
//         draw_PuppetBone(&PuppetBone->childPuppetBonesLayer1[i], PuppetBone->worldMatrix);
//     }
// }

// void draw_puppet(Puppet *puppet)
// {
//     for (uint8_t i = 0; i < puppet->puppetBonesNum; i++)
//     {
//         draw_PuppetBone(&puppet->puppetBones[i], puppet->worldMatrix);
//     }
// }

void perform(Puppet *puppet, uint8_t animationIndex, uint32_t t)
{
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
