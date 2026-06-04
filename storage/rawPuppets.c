#include "rawPuppets.h"
#include "string.h"

static const RawPuppetBone pogodynka_root_0_l1_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_head",
        .x = 0,
        .y = -34,
        .angle = -0.1f,
        .spriteIndex = 2,
        .baseSpriteAngle = 1.570796f,
        .childPuppetBonesLayer1 = NULL,
        .childPuppetBonesNumLayer1 = 0,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_head_root",
        .x = -2,
        .y = -8,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_l1_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_hand",
        .x = -22,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 5,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = NULL,
        .childPuppetBonesNumLayer1 = 0,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_wrist",
        .x = -2,
        .y = 5,
        .angle = 0.4f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_l1_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_forearm",
        .x = -32,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 4,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_arm_elbow",
        .x = 0,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_0_l1_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_arm",
        .x = -30,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 3,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_1_l1_0_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_right_forearm",
        .x = 46,
        .y = 3,
        .angle = 0.0f,
        .spriteIndex = 7,
        .baseSpriteAngle = -3.141593f,
        .childPuppetBonesLayer1 = NULL,
        .childPuppetBonesNumLayer1 = 0,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_1_l1_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_right_elbow",
        .x = 1,
        .y = 1,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_1_l1_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_l2_1_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_right_arm",
        .x = 34,
        .y = 0,
        .angle = 0.0f,
        .spriteIndex = 6,
        .baseSpriteAngle = -3.141593f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_1_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l1_0_childPuppetBonesLayer2[] = {
    {
        .label = "pogodynka_left_arm_root",
        .x = -23,
        .y = 10,
        .angle = -1.3f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
    {
        .label = "pogodynka_right_arm_root",
        .x = 23,
        .y = 9,
        .angle = 1.3f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_l2_1_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_torso",
        .x = 1,
        .y = -39,
        .angle = 0.0f,
        .spriteIndex = 1,
        .baseSpriteAngle = 1.570796f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l1_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = pogodynka_root_0_l1_0_childPuppetBonesLayer2,
        .childPuppetBonesNumLayer2 = 2,
    },
};

static const RawPuppetBone pogodynka_root_0_l2_0_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_left_leg",
        .x = 0,
        .y = 68,
        .angle = 0.0f,
        .spriteIndex = 8,
        .baseSpriteAngle = -1.570796f,
        .childPuppetBonesLayer1 = NULL,
        .childPuppetBonesNumLayer1 = 0,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_l2_1_childPuppetBonesLayer1[] = {
    {
        .label = "pogodynka_right_leg",
        .x = 0,
        .y = 68,
        .angle = 0.0f,
        .spriteIndex = 9,
        .baseSpriteAngle = -1.570796f,
        .childPuppetBonesLayer1 = NULL,
        .childPuppetBonesNumLayer1 = 0,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_root_0_childPuppetBonesLayer2[] = {
    {
        .label = "pogodynka_left_leg_root",
        .x = -12,
        .y = 36,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l2_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
    {
        .label = "pogodynka_right_leg_root",
        .x = 12,
        .y = 36,
        .angle = 0.0f,
        .spriteIndex = 255,
        .baseSpriteAngle = 0.0f,
        .childPuppetBonesLayer1 = pogodynka_root_0_l2_1_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = NULL,
        .childPuppetBonesNumLayer2 = 0,
    },
};

static const RawPuppetBone pogodynka_puppetBones[] = {
    {
        .label = "pogodynka_pelvis",
        .x = 0,
        .y = -47,
        .angle = 0.0f,
        .spriteIndex = 0,
        .baseSpriteAngle = 1.570796f,
        .childPuppetBonesLayer1 = pogodynka_root_0_childPuppetBonesLayer1,
        .childPuppetBonesNumLayer1 = 1,
        .childPuppetBonesLayer2 = pogodynka_root_0_childPuppetBonesLayer2,
        .childPuppetBonesNumLayer2 = 2,
    },
};

static const RawFrame pogodynka_anim1_track0_frames[] = {
    {.x = 257, .y = 179, .angle = 0.0f, .startFrameNum = 60},
    {.x = 188, .y = 179, .angle = 0.0f, .startFrameNum = 70},
    {.x = 188, .y = 179, .angle = 0.0f, .startFrameNum = 110},
};

static const RawAnimation pogodynka_anim1_track0_animation = {
    .frames = pogodynka_anim1_track0_frames,
    .framesNum = 3,
};

static const RawFrame pogodynka_anim1_track1_frames[] = {
    {.x = -30, .y = 0, .angle = -1.4f, .startFrameNum = 70},
};

static const RawAnimation pogodynka_anim1_track1_animation = {
    .frames = pogodynka_anim1_track1_frames,
    .framesNum = 1,
};

static const RawFrame pogodynka_anim1_track2_frames[] = {
    {.x = 0, .y = 0, .angle = 0.0f, .startFrameNum = 15},
    {.x = 0, .y = 0, .angle = 0.2f, .startFrameNum = 17},
    {.x = 0, .y = 0, .angle = 0.0f, .startFrameNum = 20},
    {.x = 0, .y = 0, .angle = 0.0f, .startFrameNum = 30},
    {.x = 0, .y = 0, .angle = 1.5f, .startFrameNum = 35},
    {.x = 0, .y = 0, .angle = 1.1f, .startFrameNum = 40},
    {.x = 0, .y = 0, .angle = 1.6f, .startFrameNum = 45},
    {.x = 0, .y = 0, .angle = 1.2f, .startFrameNum = 50},
    {.x = 0, .y = 0, .angle = 1.1999f, .startFrameNum = 60},
    {.x = 0, .y = 0, .angle = 1.6f, .startFrameNum = 77},
    {.x = 0, .y = 0, .angle = 1.2f, .startFrameNum = 100},
};

static const RawAnimation pogodynka_anim1_track2_animation = {
    .frames = pogodynka_anim1_track2_frames,
    .framesNum = 11,
};

static const RawFrame pogodynka_anim1_track3_frames[] = {
    {.x = -23, .y = 10, .angle = -1.3f, .startFrameNum = 0},
    {.x = -23, .y = 10, .angle = 0.3f, .startFrameNum = 10},
    {.x = -23, .y = 10, .angle = 0.3f, .startFrameNum = 20},
    {.x = -23, .y = 10, .angle = -1.3f, .startFrameNum = 30},
    {.x = -23, .y = 10, .angle = 0.0f, .startFrameNum = 66},
    {.x = -23, .y = 10, .angle = -1.3f, .startFrameNum = 70},
    {.x = -23, .y = 10, .angle = -0.8f, .startFrameNum = 77},
    {.x = -23, .y = 10, .angle = -0.3f, .startFrameNum = 85},
    {.x = -23, .y = 10, .angle = 0.0f, .startFrameNum = 90},
    {.x = -23, .y = 10, .angle = -1.2f, .startFrameNum = 100},
};

static const RawAnimation pogodynka_anim1_track3_animation = {
    .frames = pogodynka_anim1_track3_frames,
    .framesNum = 10,
};

static const RawFrame pogodynka_anim1_track4_frames[] = {
    {.x = -12, .y = 36, .angle = 0.0f, .startFrameNum = 60},
    {.x = -12, .y = 36, .angle = -0.2f, .startFrameNum = 63},
    {.x = -12, .y = 36, .angle = -0.4f, .startFrameNum = 66},
    {.x = -12, .y = 36, .angle = 0.0f, .startFrameNum = 70},
};

static const RawAnimation pogodynka_anim1_track4_animation = {
    .frames = pogodynka_anim1_track4_frames,
    .framesNum = 4,
};

static const RawFrame pogodynka_anim1_track5_frames[] = {
    {.x = 23, .y = 9, .angle = 1.3f, .startFrameNum = 45},
    {.x = 23, .y = 9, .angle = 1.6f, .startFrameNum = 50},
    {.x = 23, .y = 9, .angle = 1.3f, .startFrameNum = 67},
};

static const RawAnimation pogodynka_anim1_track5_animation = {
    .frames = pogodynka_anim1_track5_frames,
    .framesNum = 3,
};

static const RawFrame pogodynka_anim1_track6_frames[] = {
    {.x = 1, .y = 1, .angle = 0.0f, .startFrameNum = 45},
    {.x = 1, .y = 1, .angle = 1.1f, .startFrameNum = 50},
    {.x = 1, .y = 1, .angle = 0.1f, .startFrameNum = 67},
};

static const RawAnimation pogodynka_anim1_track6_animation = {
    .frames = pogodynka_anim1_track6_frames,
    .framesNum = 3,
};

static const RawFrame pogodynka_anim1_track7_frames[] = {
    {.x = 12, .y = 36, .angle = 0.0f, .startFrameNum = 60},
    {.x = 12, .y = 36, .angle = 0.3f, .startFrameNum = 63},
    {.x = 12, .y = 36, .angle = 0.5f, .startFrameNum = 66},
    {.x = 12, .y = 36, .angle = 0.0f, .startFrameNum = 70},
};

static const RawAnimation pogodynka_anim1_track7_animation = {
    .frames = pogodynka_anim1_track7_frames,
    .framesNum = 4,
};

static const RawBoneAnimationPair pogodynka_anim1_boneAnimationPairs[] = {
    {.rawBone = NULL, .rawAnimation = &pogodynka_anim1_track0_animation}, /* root puppet */
    {.rawBone = &pogodynka_root_0_l1_0_l2_0_childPuppetBonesLayer1[0], .rawAnimation = &pogodynka_anim1_track1_animation},
    {.rawBone = &pogodynka_root_0_l1_0_l2_0_l1_0_childPuppetBonesLayer1[0], .rawAnimation = &pogodynka_anim1_track2_animation},
    {.rawBone = &pogodynka_root_0_l1_0_childPuppetBonesLayer2[0], .rawAnimation = &pogodynka_anim1_track3_animation},
    {.rawBone = &pogodynka_root_0_childPuppetBonesLayer2[0], .rawAnimation = &pogodynka_anim1_track4_animation},
    {.rawBone = &pogodynka_root_0_l1_0_childPuppetBonesLayer2[1], .rawAnimation = &pogodynka_anim1_track5_animation},
    {.rawBone = &pogodynka_root_0_l1_0_l2_1_l1_0_childPuppetBonesLayer1[0], .rawAnimation = &pogodynka_anim1_track6_animation},
    {.rawBone = &pogodynka_root_0_childPuppetBonesLayer2[1], .rawAnimation = &pogodynka_anim1_track7_animation},
};

const RawPuppet pogodynka = {
    .label = "pogodynkaRoot",
    .x = 257,
    .y = 179,
    .angle = 0.0f,
    .puppetBones = pogodynka_puppetBones,
    .puppetBonesNum = 1,
    .boneAnimationPairs = pogodynka_anim1_boneAnimationPairs,
    .boneAnimationPairsNum = 8,
};

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
const RawPuppet rawPuppets[1] = {
    pogodynka,
};
#else
const RawPuppet rawPuppets[1] = {
    pogodynka,
};
#endif
