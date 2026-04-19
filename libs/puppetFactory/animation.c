#include "animation.h"

static const AnimationKeyframe jawAnimationTrack0Keyframes[3] = {
    {.x = 1.0f, .y = 47.0f, .angle = 0.0f, .timelineFrame = 0},
    {.x = -4.0f, .y = 48.0f, .angle = 0.4f, .timelineFrame = 5},
    {.x = 1.0f, .y = 47.0f, .angle = 0.0f, .timelineFrame = 10},
};

static const AnimationTrack jawAnimationTracks[1] = {
    {
        .boneLabel = "mascotSkullJawParent",
        .keyframes = jawAnimationTrack0Keyframes,
        .keyframesNum = 3,
    },
};

static const AnimationKeyframe raisingArmParentTrack0Keyframes[3] = {
    {.x = 9.0f, .y = 3.0f, .angle = 0.0f, .timelineFrame = 0},
    {.x = 9.0f, .y = 3.0f, .angle = -2.1f, .timelineFrame = 10},
    {.x = 9.0f, .y = 3.0f, .angle = 0.0f, .timelineFrame = 20},
};

static const AnimationKeyframe raisingArmParentTrack1Keyframes[3] = {
    {.x = 22.0f, .y = 78.0f, .angle = 1.14f, .timelineFrame = 0},
    {.x = 22.0f, .y = 78.0f, .angle = -0.06f, .timelineFrame = 10},
    {.x = 22.0f, .y = 78.0f, .angle = 1.14f, .timelineFrame = 20},
};

static const AnimationTrack raisingArmParentTracks[2] = {
    {
        .boneLabel = "mascotArmElbow",
        .keyframes = raisingArmParentTrack0Keyframes,
        .keyframesNum = 3,
    },
    {
        .boneLabel = "mascotArmParent",
        .keyframes = raisingArmParentTrack1Keyframes,
        .keyframesNum = 3,
    },
};

static const AnimationKeyframe wavingTrack0Keyframes[3] = {
    {.x = 144.0f, .y = 189.0f, .angle = -2.0f, .timelineFrame = 0},
    {.x = 144.0f, .y = 189.0f, .angle = -1.0f, .timelineFrame = 10},
    {.x = 144.0f, .y = 189.0f, .angle = -2.0f, .timelineFrame = 20},
};

static const AnimationTrack wavingTracks[1] = {
    {
        .boneLabel = "",
        .keyframes = wavingTrack0Keyframes,
        .keyframesNum = 3,
    },
};

static const AnimationClip animationClips[3] = {
    {
        .label = "jawAnimation",
        .tracks = jawAnimationTracks,
        .tracksNum = 1,
        .durationFrames = 10,
    },
    {
        .label = "raisingArmParent",
        .tracks = raisingArmParentTracks,
        .tracksNum = 2,
        .durationFrames = 20,
    },
    {
        .label = "waving",
        .tracks = wavingTracks,
        .tracksNum = 1,
        .durationFrames = 20,
    },
};

const AnimationClip *get_animation_clip_by_index(uint8_t index)
{
    return &animationClips[index];
}

uint8_t get_animation_clips_num(void)
{
    return (uint8_t)(sizeof(animationClips) / sizeof(animationClips[0]));
}
