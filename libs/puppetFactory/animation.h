#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdint.h>

typedef struct
{
    float x;
    float y;
    float angle;
    uint16_t timelineFrame;
} AnimationKeyframe;

typedef struct
{
    const char *boneLabel;
    const AnimationKeyframe *keyframes;
    uint16_t keyframesNum;
} AnimationTrack;

typedef struct
{
    const char *label;
    const AnimationTrack *tracks;
    uint8_t tracksNum;
    uint16_t durationFrames;
} AnimationClip;

const AnimationClip *get_animation_clip_by_index(uint8_t index);
uint8_t get_animation_clips_num(void);

#endif
