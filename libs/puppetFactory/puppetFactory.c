#include "IPuppetFactory.h"
#include "puppetFactory.h"
#include "puppet.h"
#include "string.h"
#include "../storage/rawPuppets.h"

Bone *create_bones(const RawBone *rawBones, const uint8_t bonesNum, int *parentWorldMatrix)
{
    Bone *newBones = (Bone *)malloc(sizeof(Bone) * bonesNum);
    for (uint8_t i = 0; i < bonesNum; i++)
    {
        newBones[i].label = rawBones[i].label;
        newBones[i].x = rawBones[i].x;
        newBones[i].y = rawBones[i].y;
        newBones[i].angle = rawBones[i].angle;
        newBones[i].sprite = get_sprite(rawBones[i].spriteIndex);
        newBones[i].baseSpriteAngle = float_to_fixed(rawBones[i].baseSpriteAngle);
        make_local_matrix(&newBones[i]);
        make_world_matrix(&newBones[i], parentWorldMatrix);
        newBones[i].childBonesNumLayer1 = rawBones[i].childBonesNumLayer1;
        newBones[i].childBonesNumLayer2 = rawBones[i].childBonesNumLayer2;
        if (rawBones[i].childBonesNumLayer1 != 0)
            newBones[i].childBonesLayer1 = create_bones(rawBones[i].childBonesLayer1, rawBones[i].childBonesNumLayer1, newBones->worldMatrix);
        if (rawBones[i].childBonesNumLayer2 != 0)
            newBones[i].childBonesLayer2 = create_bones(rawBones[i].childBonesLayer2, rawBones[i].childBonesNumLayer2, newBones->worldMatrix);
    }
    return newBones;
}

Puppet *create_puppet(uint8_t puppetIndex)
{
    Puppet *newPuppet = (Puppet *)malloc(sizeof(Puppet));
    const RawPuppet *rawPuppet = get_raw_puppet(puppetIndex);
    newPuppet->label = rawPuppet->label;
    newPuppet->x = rawPuppet->x;
    newPuppet->y = rawPuppet->y;
    newPuppet->angle = rawPuppet->angle;
    newPuppet->bonesNum = rawPuppet->bonesNum;
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
    if (rawPuppet->bonesNum != 0)
        newPuppet->bones = create_bones(rawPuppet->bones, rawPuppet->bonesNum, newPuppet->worldMatrix);
    return newPuppet;
}

static IPuppetFactory puppet = {
    .create_puppet = create_puppet,
};

const IPuppetFactory *get_puppetFactory(void)
{
    return &puppet;
}