#include "IPuppetFactory.h"
#include "puppetFactory.h"
#include "puppetCalc.h"
#include "puppet.h"
#include "string.h"
#include "../storage/rawPuppets.h"

static const IStorage *_storage;

void init_puppet_factory(const IStorage * storage)
{
    _storage = storage;
}

PuppetBone *create_PuppetBones(const RawPuppetBone *rawPuppetBones, const uint8_t puppetBonesNum, int *parentWorldMatrix)
{
    PuppetBone *newPuppetBones = (PuppetBone *)malloc(sizeof(PuppetBone) * puppetBonesNum);
    for (uint8_t i = 0; i < puppetBonesNum; i++)
    {
        newPuppetBones[i].x = rawPuppetBones[i].x;
        newPuppetBones[i].y = rawPuppetBones[i].y;
        newPuppetBones[i].angle = rawPuppetBones[i].angle;
        newPuppetBones[i].sprite = _storage->get_sprite(rawPuppetBones[i].spriteIndex);
        newPuppetBones[i].baseSpriteAngle = float_to_fixed(rawPuppetBones[i].baseSpriteAngle);
        make_local_matrix(&newPuppetBones[i]);
        make_world_matrix(&newPuppetBones[i], parentWorldMatrix);
        newPuppetBones[i].childPuppetBonesNumLayer1 = rawPuppetBones[i].childPuppetBonesNumLayer1;
        newPuppetBones[i].childPuppetBonesNumLayer2 = rawPuppetBones[i].childPuppetBonesNumLayer2;
        if (rawPuppetBones[i].childPuppetBonesNumLayer1 != 0)
            newPuppetBones[i].childPuppetBonesLayer1 = create_PuppetBones(rawPuppetBones[i].childPuppetBonesLayer1, rawPuppetBones[i].childPuppetBonesNumLayer1, newPuppetBones->worldMatrix);
        if (rawPuppetBones[i].childPuppetBonesNumLayer2 != 0)
            newPuppetBones[i].childPuppetBonesLayer2 = create_PuppetBones(rawPuppetBones[i].childPuppetBonesLayer2, rawPuppetBones[i].childPuppetBonesNumLayer2, newPuppetBones->worldMatrix);
    }
    return newPuppetBones;
}

Puppet *create_puppet(uint8_t puppetIndex)
{
    Puppet *newPuppet = (Puppet *)malloc(sizeof(Puppet));
    const RawPuppet *rawPuppet = _storage->get_raw_puppet(puppetIndex);
    newPuppet->x = rawPuppet->x;
    newPuppet->y = rawPuppet->y;
    newPuppet->angle = rawPuppet->angle;
    newPuppet->puppetBonesNum = rawPuppet->puppetBonesNum;
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
    memcpy(newPuppet->worldMatrix,newPuppet->localMatrix,sizeof(newPuppet->localMatrix));
    if (rawPuppet->puppetBonesNum != 0)
        newPuppet->puppetBones = create_PuppetBones(rawPuppet->puppetBones, rawPuppet->puppetBonesNum, newPuppet->worldMatrix);
    return newPuppet;
}

static IPuppetFactory puppet = {
    .init_puppet_factory = init_puppet_factory,
    .create_puppet = create_puppet,
};

const IPuppetFactory *get_puppetFactory(void)
{
    return &puppet;
}
