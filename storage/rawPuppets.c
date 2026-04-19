#include "rawPuppets.h"

static const RawBone mascotSkullJawParentChildBonesLayer1[1] = {
    {
        .label = "mascotSkullJaw",
        .x = 0,
        .y = -18,
        .angle = 0.0f,
        .spriteIndex = 7,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotSkullLeftEyeParentChildBonesLayer1[1] = {
    {
        .label = "mascotSkullLeftEye",
        .x = 0,
        .y = -14,
        .angle = 0.0f,
        .spriteIndex = 9,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotSkullRightEyeParentChildBonesLayer1[1] = {
    {
        .label = "mascotSkullRightEye",
        .x = 0,
        .y = -13,
        .angle = 0.0f,
        .spriteIndex = 10,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotReflectionParentChildBonesLayer1[1] = {
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

static const RawBone mascotWristChildBonesLayer1[1] = {
    {
        .label = "mascotHand",
        .x = 34,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 3,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotArm1ChildBonesLayer2[1] = {
    {
        .label = "mascotWrist",
        .x = 0,
        .y = 3,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotWristChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotArmElbowChildBonesLayer2[1] = {
    {
        .label = "mascotArm1",
        .x = 44,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 1,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArm1ChildBonesLayer2,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotArm2ChildBonesLayer2[1] = {
    {
        .label = "mascotArmElbow",
        .x = 9,
        .y = 3,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArmElbowChildBonesLayer2,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotArmParentChildBonesLayer2[1] = {
    {
        .label = "mascotArm2",
        .x = 28,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 2,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArm2ChildBonesLayer2,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotSkullChildBonesLayer2[4] = {
    {
        .label = "mascotSkullJawParent",
        .x = 1,
        .y = 47,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotSkullJawParentChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
    {
        .label = "mascotSkullEyesBack",
        .x = 0,
        .y = 45,
        .angle = 0.0f,
        .spriteIndex = 8,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
    {
        .label = "mascotSkullLeftEyeParent",
        .x = 10,
        .y = 32,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotSkullLeftEyeParentChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
    {
        .label = "mascotSkullRightEyeParent",
        .x = -1,
        .y = 31,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotSkullRightEyeParentChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotSkullParentChildBonesLayer1[1] = {
    {
        .label = "mascotSkull",
        .x = 0,
        .y = -40,
        .angle = 0.0f,
        .spriteIndex = 6,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotSkullChildBonesLayer2,
        .childBonesNumLayer2 = 4,
    },
};

static const RawBone mascotBoneChildBonesLayer1[2] = {
    {
        .label = "mascotSkullParent",
        .x = 0,
        .y = 55,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotSkullParentChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
    {
        .label = "mascotReflectionParent",
        .x = 13,
        .y = 49,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = mascotReflectionParentChildBonesLayer1,
        .childBonesNumLayer1 = 1,
        .childBonesLayer2 = NULL,
        .childBonesNumLayer2 = 0,
    },
};

static const RawBone mascotBoneChildBonesLayer2[1] = {
    {
        .label = "mascotArmParent",
        .x = 22,
        .y = 78,
        .angle = 1.14f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childBonesLayer1 = NULL,
        .childBonesNumLayer1 = 0,
        .childBonesLayer2 = mascotArmParentChildBonesLayer2,
        .childBonesNumLayer2 = 1,
    },
};

static const RawBone mascotRootBones[1] = {
    {
        .label = "mascotBone",
        .x = 0,
        .y = -220,
        .angle = 0.0f,
        .spriteIndex = 4,
        .baseSpriteAngle = 1.57f,
        .childBonesLayer1 = mascotBoneChildBonesLayer1,
        .childBonesNumLayer1 = 2,
        .childBonesLayer2 = mascotBoneChildBonesLayer2,
        .childBonesNumLayer2 = 1,
    },
};

const RawPuppet rawPuppets[1] = {
    {
        .label = "mascotRoot",
        .x = 120,
        .y = 250,
        .angle = 0.0f,
        .bones = mascotRootBones,
        .bonesNum = 1,
    },
};
