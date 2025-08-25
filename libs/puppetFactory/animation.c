#include "animation.h"

static const Frame jawAnimationFrames[10] = {
    {.x = 1, .y = 0, .angle = -0.08f},
    {.x = 1, .y = 0, .angle = -0.08f},
    {.x = 1, .y = -1, .angle = -0.08f},
    {.x = 1, .y = 0, .angle = -0.08f},
    {.x = 1, .y = -1, .angle = -0.08f},
    {.x = -1, .y = 1, .angle = 0.08f},
    {.x = -1, .y = 0, .angle = 0.08f},
    {.x = -1, .y = 1, .angle = 0.08f},
    {.x = -1, .y = 0, .angle = 0.08f},
    {.x = -1, .y = 0, .angle = 0.08f},
};

static const Frame skullAnimationFrames[10] = {
    {.x = 0, .y = 0, .angle = 0.0f},
    {.x = 0, .y = 0, .angle = 0.0f},
    {.x = 0, .y = -1, .angle = 0.0f},
    {.x = 0, .y = 0, .angle = 0.0f},
    {.x = 0, .y = -1, .angle = 0.0f},
    {.x = 0, .y = 1, .angle = 0.0f},
    {.x = 0, .y = 0, .angle = 0.0f},
    {.x = 0, .y = 1, .angle = 0.0f},
    {.x = 0, .y = 0, .angle = 0.0f},
    {.x = 0, .y = 0, .angle = 0.0f},
};

const Frame raisingArmParentFrames[15] = {
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
    {.x = 0, .y = 0, .angle = -0.076f},
};

static const Frame raisingElbowFrames[15] = {
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
    {.x = 0, .y = 0, .angle = -0.1333f},
};

const Frame wavingFrames[20] = {
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = 0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
    {.x = 0, .y = 0, .angle = -0.1f},
};

static const Animation animations[5] = {
    {
        .label = "jawAnimation",
        .frames = jawAnimationFrames,
        .framesNum = 10,
    },
    {
        .label = "skullAnimation",
        .frames = skullAnimationFrames,
        .framesNum = 10,
    },
    {
        .label = "raisingArmParent",
        .frames = raisingArmParentFrames,
        .framesNum = 15,
    },
    {
        .label = "raisingElbow",
        .frames = raisingElbowFrames,
        .framesNum = 15,
    },
    {
        .label = "waving",
        .frames = wavingFrames,
        .framesNum = 20,
    },
};

const Animation *get_animation_by_index(uint8_t index)
{
    return &animations[index];
}