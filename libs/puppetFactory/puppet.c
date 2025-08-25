#include "puppet.h"
#include <string.h>

void make_local_matrix(Bone *bone)
{
    int32_t angleIndex = radian_to_index(float_to_fixed(bone->angle));
    int16_t sin = fast_sin(angleIndex);
    int16_t cos = fast_cos(angleIndex);
    bone->localMatrix[0] = cos;
    bone->localMatrix[1] = -sin;
    bone->localMatrix[2] = bone->x << SHIFT_FACTOR;
    bone->localMatrix[3] = sin;
    bone->localMatrix[4] = cos;
    bone->localMatrix[5] = bone->y << SHIFT_FACTOR;
    bone->localMatrix[6] = 0;
    bone->localMatrix[7] = 0;
    bone->localMatrix[8] = SCALE_FACTOR;
}

void make_world_matrix(Bone *bone, int *parentWorldMatrix)
{
    int *result = mul_matrices(parentWorldMatrix, bone->localMatrix, 3, 3);
    for (uint8_t i = 0; i < 9; i++)
    {
        bone->worldMatrix[i] = result[i];
    }
    free(result);
}

void update_bones_world_matrices(Bone *bone, int *parentWorldMatrix)
{
    make_local_matrix(bone);
    make_world_matrix(bone, parentWorldMatrix);
    for (uint8_t i = 0; i < bone->childBonesNumLayer1; i++)
    {
        update_bones_world_matrices(&bone->childBonesLayer1[i], bone->worldMatrix);
    }
    for (uint8_t i = 0; i < bone->childBonesNumLayer2; i++)
    {
        update_bones_world_matrices(&bone->childBonesLayer2[i], bone->worldMatrix);
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
    for (uint8_t i = 0; i < puppet->bonesNum; i++)
    {
        update_bones_world_matrices(&puppet->bones[i], puppet->worldMatrix);
    }
}

void move_puppet(Puppet *puppet, int16_t newX, int16_t newY)
{
    puppet->x += newX;
    puppet->y += newY;
    update_world_matrices(puppet);
}

Bone *get_bone_by_name(Bone *bone, const char *boneLabel)
{
    if (strcmp(bone->label, boneLabel) == 0)
        return bone;

    for (uint8_t i = 0; i < bone->childBonesNumLayer1; i++)
    {
        Bone *result = get_bone_by_name(&bone->childBonesLayer1[i], boneLabel);
        if (result)
            return result;
    }

    for (uint8_t i = 0; i < bone->childBonesNumLayer2; i++)
    {
        Bone *result = get_bone_by_name(&bone->childBonesLayer2[i], boneLabel);
        if (result)
            return result;
    }
    return NULL;
}

void transform_bone(Bone *bone, int16_t x, int16_t y, float angle)
{
    if (!bone)
        return;
    bone->x += x;
    bone->y += y;
    bone->angle += angle;
}

const Animation *get_animation_by_label(char *label)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        const Animation *animation = get_animation_by_index(i);
        if (strcmp(animation->label, label) == 0)
            return animation;
    }
    return NULL;
}

void animate_bones(BoneAnimation *boneAnimations, uint8_t animationsNum, uint32_t frameNum, bool invert)
{
    for (uint8_t i = 0; i < animationsNum; i++)
    {
        uint16_t animationFramesNum = boneAnimations[i].animation->framesNum;
        while (frameNum >= animationFramesNum)
            frameNum -= animationFramesNum;
        if (invert)
            frameNum = animationFramesNum - 1 - frameNum;
        const Frame *frame = &boneAnimations[i].animation->frames[frameNum];
        if (invert)
            transform_bone(boneAnimations[i].bone, -frame->x, -frame->y, -frame->angle);
        else
            transform_bone(boneAnimations[i].bone, frame->x, frame->y, frame->angle);
    }
}

void change_sprite(Bone *bone, const Sprite *newSprite)
{
    bone->sprite = newSprite;
}