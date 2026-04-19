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

static Bone *get_puppet_bone_by_name(Puppet *puppet, const char *boneLabel)
{
    if (puppet == NULL || boneLabel == NULL || boneLabel[0] == '\0')
        return NULL;

    for (uint8_t i = 0; i < puppet->bonesNum; i++)
    {
        Bone *result = get_bone_by_name(&puppet->bones[i], boneLabel);
        if (result != NULL)
            return result;
    }
    return NULL;
}

static int16_t round_to_int16(float value)
{
    if (value >= 0.0f)
        return (int16_t)(value + 0.5f);
    return (int16_t)(value - 0.5f);
}

static void set_bone_transform(Bone *bone, const BoneTransform *transform)
{
    if (bone == NULL || transform == NULL)
        return;
    bone->x = round_to_int16(transform->x);
    bone->y = round_to_int16(transform->y);
    bone->angle = transform->angle;
}

static void set_puppet_transform(Puppet *puppet, const BoneTransform *transform)
{
    if (puppet == NULL || transform == NULL)
        return;
    puppet->x = round_to_int16(transform->x);
    puppet->y = round_to_int16(transform->y);
    puppet->angle = transform->angle;
}

static BoneTransform sample_track_transform(const AnimationTrack *track, uint16_t frameNum)
{
    BoneTransform sampled = {0.0f, 0.0f, 0.0f};
    if (track == NULL || track->keyframes == NULL || track->keyframesNum == 0)
        return sampled;

    const AnimationKeyframe *keyframes = track->keyframes;
    if (track->keyframesNum == 1 || frameNum <= keyframes[0].timelineFrame)
    {
        sampled.x = keyframes[0].x;
        sampled.y = keyframes[0].y;
        sampled.angle = keyframes[0].angle;
        return sampled;
    }

    for (uint16_t i = 0; i + 1 < track->keyframesNum; i++)
    {
        const AnimationKeyframe *from = &keyframes[i];
        const AnimationKeyframe *to = &keyframes[i + 1];
        if (frameNum > to->timelineFrame)
            continue;

        uint16_t span = (uint16_t)(to->timelineFrame - from->timelineFrame);
        if (span == 0)
        {
            sampled.x = to->x;
            sampled.y = to->y;
            sampled.angle = to->angle;
            return sampled;
        }

        float alpha = (float)(frameNum - from->timelineFrame) / (float)span;
        sampled.x = from->x + (to->x - from->x) * alpha;
        sampled.y = from->y + (to->y - from->y) * alpha;
        sampled.angle = from->angle + (to->angle - from->angle) * alpha;
        return sampled;
    }

    const AnimationKeyframe *last = &keyframes[track->keyframesNum - 1];
    sampled.x = last->x;
    sampled.y = last->y;
    sampled.angle = last->angle;
    return sampled;
}

const AnimationClip *get_animation_clip_by_label(const char *label)
{
    uint8_t clipsNum = get_animation_clips_num();
    for (uint8_t i = 0; i < clipsNum; i++)
    {
        const AnimationClip *clip = get_animation_clip_by_index(i);
        if (clip != NULL && strcmp(clip->label, label) == 0)
            return clip;
    }
    return NULL;
}

void animate_clip(Puppet *puppet, const AnimationClip *clip, uint32_t frameNum, bool invert)
{
    if (puppet == NULL || clip == NULL || clip->tracks == NULL || clip->tracksNum == 0)
        return;

    uint16_t sampledFrame = 0;
    if (clip->durationFrames > 0)
    {
        sampledFrame = (uint16_t)(frameNum % clip->durationFrames);
        if (invert)
            sampledFrame = (uint16_t)(clip->durationFrames - 1u - sampledFrame);
    }

    for (uint8_t i = 0; i < clip->tracksNum; i++)
    {
        const AnimationTrack *track = &clip->tracks[i];
        BoneTransform sampledTransform = sample_track_transform(track, sampledFrame);

        if (track->boneLabel == NULL || track->boneLabel[0] == '\0')
        {
            set_puppet_transform(puppet, &sampledTransform);
            continue;
        }

        Bone *targetBone = get_puppet_bone_by_name(puppet, track->boneLabel);
        set_bone_transform(targetBone, &sampledTransform);
    }
}

void change_sprite(Bone *bone, const Sprite *newSprite)
{
    bone->sprite = newSprite;
}
