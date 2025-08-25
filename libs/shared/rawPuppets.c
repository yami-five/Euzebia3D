#include "rawPuppets.h"
#include "string.h"



static const RawBone mascotSkullJaw[1] = {
    {
        .label = "mascotSkullJaw",
        .x = 0,
        .y = -18,
        .angle = 0.0f,
        .spriteIndex = 7,
        .baseSpriteAngle = 1.57f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
};
static const RawBone mascotSkullLeftEye[1] = {
    {
        .label = "mascotSkullLeftEye",
        .x = 0,
        .y = -14,
        .angle = 0.0f,
        .spriteIndex = 9,
        .baseSpriteAngle = 1.57f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
};
static const RawBone mascotSkullRightEye[1] = {
    {
        .label = "mascotSkullRightEye",
        .x = 0,
        .y = -13,
        .angle = 0.0f,
        .spriteIndex = 10,
        .baseSpriteAngle = 1.57f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
};

static const RawBone mascotSkullChildren[4] = {
    {
        .label = "mascotSkullJawParent",
        .x = 0,
        .y = 47,
        .angle = 0.0f,
        .spriteIndex = 255,
        .angle = 0.0f,
        .childBonesNumLayer1 = 1,
        .childBonesLayer1 = mascotSkullJaw,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
    {
        .label = "mascotSkullEyesBack",
        .x = 0,
        .y = 45,
        .angle = 0.0f,
        .spriteIndex = 8,
        .angle = 0.0f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
    {
        .label = "mascotSkullLeftEyeParent",
        .x = 10,
        .y = 32,
        .angle = 0.0f,
        .spriteIndex = 255,
        .angle = 0.0f,
        .childBonesNumLayer1 = 1,
        .childBonesLayer1 = mascotSkullLeftEye,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
    {
        .label = "mascotSkullRightEyeParent",
        .x = -1,
        .y = 31,
        .angle = 0.0f,
        .spriteIndex = 255,
        .angle = 0.0f,
        .childBonesNumLayer1 = 1,
        .childBonesLayer1 = mascotSkullRightEye,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
};

static const RawBone mascotSkull[1] = {
    {
        .label = "mascotSkull",
        .x = 0,
        .y = -40,
        .angle = 0.0f,
        .spriteIndex = 6,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotSkullChildren,
        .childBonesNumLayer2 = 4,
    },
};

static const RawBone mascotReflection[1] = {
    {
        .label = "mascotReflection",
        .x = 0,
        .y = -25,
        .angle = 0.0f,
        .spriteIndex = 5,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotArmHand[1] = {
    {
        .label = "mascotArmHand",
        .x = 34,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 3,
        .baseSpriteAngle = 0.0f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 0,
        .childBonesLayer2 = NULL,
    },
};

static const RawBone mascotWrist[1] = {
    {
        .label = "mascotArmWrist",
        .x = 0,
        .y = 3,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 1,
        .childBonesLayer2 = mascotArmHand,
    },
};

static const RawBone mascotArmForearm[1] = {
    {
        .label = "mascotArmForearm",
        .x = 44,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 1,
        .baseSpriteAngle = 0.0f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 1,
        .childBonesLayer2 = mascotWrist,
    },
};

static const RawBone mascotArmElbow[1] = {
    {
        .label = "mascotArmElbow",
        .x = 12,
        .y = 5,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesNumLayer1 = 0,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer2 = 1,
        .childBonesLayer2 = mascotArmForearm,
    },
};

static const RawBone mascotBodyChildrenLayer1[2] = {
    {
        .label = "mascotSkullParent",
        .x = 0,
        .y = 55,
        .spriteIndex = 255,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotSkull,
        .childBonesNumLayer2 = 1,
    },
    {
        .label = "mascotReflectionParent",
        .x = 13,
        .y = 49,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotReflection,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotArmMain[1] = {
    {
        .label = "mascotArm",
        .x = 28,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 2,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArmElbow,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotBodyChildrenLayer2[1] = {
    {
        .label = "mascotArmParent",
        .x = 24,
        .y = 79,
        .angle = 1.14f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArmMain,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotBodyBone[1] = {
    {.label = "mascotBone",
     .x = 0,
     .y = -220,
     .angle = 0.0f,
     .spriteIndex = 4,
     .baseSpriteAngle = 1.57f,
     .childBonesLayer1 = mascotBodyChildrenLayer1,
     .childBonesNumLayer1 = 2,
     .childBonesLayer2 = mascotBodyChildrenLayer2,
     .childBonesNumLayer2 = 1},
};

static const RawBone moonBone[1] = {
    {.label = "moonBone",
     .x = 0,
     .y = -65,
     .angle = 0.0f,
     .spriteIndex = 26,
     .baseSpriteAngle = 0.96f,
     .childBonesLayer1 = NULL,
     .childBonesNumLayer1 = 0,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
};

static const RawBone rocketBone[1] = {
    {.label = "rocketBone",
     .x = 0,
     .y = 65,
     .angle = 0.0f,
     .spriteIndex = 27,
     .baseSpriteAngle = 4.10f,
     .childBonesLayer1 = NULL,
     .childBonesNumLayer1 = 0,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
};

static const RawBone logoChildrenLayer1[2] = {
    {.label = "moonParent",
     .x = -75,
     .y = 0,
     .angle = 0.0f,
     .spriteIndex = 255,
     .baseSpriteAngle = 0.0f,
     .childBonesLayer1 = moonBone,
     .childBonesNumLayer1 = 1,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
    {.label = "rocketParent",
     .x = 75,
     .y = 0,
     .angle = 0.0f,
     .spriteIndex = 255,
     .baseSpriteAngle = 0.0f,
     .childBonesLayer1 = rocketBone,
     .childBonesNumLayer1 = 1,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
};

static const RawBone logoCenter[1] = {
    {.label = "logoCenter",
     .x = 0,
     .y = 65,
     .angle = -25.9f,
     .spriteIndex = 255,
     .baseSpriteAngle = 0.0f,
     .childBonesLayer1 = logoChildrenLayer1,
     .childBonesNumLayer1 = 2,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
};

static const RawBone logoBone[1] = {
    {.label = "logoBone",
     .x = 0,
     .y = -130,
     .angle = 0.0f,
     .spriteIndex = 25,
     .baseSpriteAngle = 0.0f,
     .childBonesLayer1 = logoCenter,
     .childBonesNumLayer1 = 1,
     .childBonesLayer2 = NULL,
     .childBonesNumLayer2 = 0},
};

static const RawPuppet rawPuppets[2] = {
    {
        .label = "mascotRoot",
        .x = 0,
        .y = 0,
        .angle = 0.0f,
        .bonesNum = 1,
        .bones = mascotBodyBone,
    },
    {
        .label = "logoRoot",
        .x = 120,
        .y = 225,
        .angle = 0.0f,
        .bonesNum = 1,
        .bones = logoBone,
    },
};

const RawPuppet *get_raw_puppet(uint8_t puppetIndex)
{
    return &rawPuppets[puppetIndex];
}