#include "puppetCalc.h"
#include <string.h>

void make_local_matrix(PuppetBone *PuppetBone)
{
    int32_t angleIndex = radian_to_index(float_to_fixed(PuppetBone->angle));
    int16_t sin = fast_sin(angleIndex);
    int16_t cos = fast_cos(angleIndex);
    PuppetBone->localMatrix[0] = cos;
    PuppetBone->localMatrix[1] = -sin;
    PuppetBone->localMatrix[2] = PuppetBone->x << SHIFT_FACTOR;
    PuppetBone->localMatrix[3] = sin;
    PuppetBone->localMatrix[4] = cos;
    PuppetBone->localMatrix[5] = PuppetBone->y << SHIFT_FACTOR;
    PuppetBone->localMatrix[6] = 0;
    PuppetBone->localMatrix[7] = 0;
    PuppetBone->localMatrix[8] = SCALE_FACTOR;
}

void make_world_matrix(PuppetBone *PuppetBone, int *parentWorldMatrix)
{
    int *result = mul_matrices(parentWorldMatrix, PuppetBone->localMatrix, 3, 3);
    for (uint8_t i = 0; i < 9; i++)
    {
        PuppetBone->worldMatrix[i] = result[i];
    }
    free(result);
}

void update_PuppetBones_world_matrices(PuppetBone *PuppetBone, int *parentWorldMatrix)
{
    make_local_matrix(PuppetBone);
    make_world_matrix(PuppetBone, parentWorldMatrix);
    for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer1; i++)
    {
        update_PuppetBones_world_matrices(&PuppetBone->childPuppetBonesLayer1[i], PuppetBone->worldMatrix);
    }
    for (uint8_t i = 0; i < PuppetBone->childPuppetBonesNumLayer2; i++)
    {
        update_PuppetBones_world_matrices(&PuppetBone->childPuppetBonesLayer2[i], PuppetBone->worldMatrix);
    }
}

void update_world_matrices(Puppet *puppet)
{
    int32_t angleIndex = radian_to_index(float_to_fixed(puppet->angle));
    int16_t sin = fast_sin(angleIndex);
    int16_t cos = fast_cos(angleIndex);
    puppet->localMatrix[0] = cos;
    puppet->localMatrix[1] = -sin;
    puppet->localMatrix[2] = puppet->x << SHIFT_FACTOR;
    puppet->localMatrix[3] = sin;
    puppet->localMatrix[4] = cos;
    puppet->localMatrix[5] = puppet->y << SHIFT_FACTOR;
    puppet->localMatrix[6] = puppet->localMatrix[7] = 0;
    puppet->localMatrix[8] = SCALE_FACTOR;
    memcpy(puppet->worldMatrix, puppet->localMatrix, sizeof(puppet->localMatrix));
    for (uint8_t i = 0; i < puppet->puppetBonesNum; i++)
    {
        update_PuppetBones_world_matrices(&puppet->puppetBones[i], puppet->worldMatrix);
    }
}