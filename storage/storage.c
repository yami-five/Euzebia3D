#include "storage.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "lookup_tables.h"

#if defined(EUZEBIA3D_USE_PSRAM) && defined(EUZEBIA3D_PLATFORM_PICO) && defined(PICO_RP2350) && (PICO_RP2350)
#include "hardware/flash.h"
#include "hardware/regs/xip.h"
#include "hardware/structs/xip.h"
#define EUZEBIA3D_PSRAM_RUNTIME_ENABLED 1
#else
#define EUZEBIA3D_PSRAM_RUNTIME_ENABLED 0
#endif

// Single translation unit for all storage implementation.
#include "fonts.c"
#include "gfx.c"
#include "post_processing.c"
#include "rawPuppets.c"
#include "scrollers.c"
#include "sprites.c"

typedef struct
{
    const uint16_t *pixels;
    uint8_t height;
    uint8_t width;
    bool canRotate;
} SpriteRuntime;

typedef struct
{
    const Sprite *sprite;
    uint8_t width;
} FontRuntime;

typedef struct
{
    const uint16_t *image;
    uint32_t size;
    uint16_t width;
    uint16_t heigth;
} ImageRuntime;

typedef struct
{
    const uint16_t *bitmap;
    uint8_t width;
    uint8_t height;
} ScrollerRuntime;

typedef struct
{
    int x;
    int y;
    float angle;
    int startFrameNum;
} KeyFrameRuntime;

typedef struct
{
    const KeyFrameRuntime *keyFrames;
    uint16_t keyFramesNum;
} PuppetBoneAnimTimelineRuntime;

typedef struct RawPuppetBoneRuntime RawPuppetBoneRuntime;
struct RawPuppetBoneRuntime
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    uint8_t spriteIndex;
    float baseSpriteAngle;
    const RawPuppetBoneRuntime *childPuppetBonesLayer1;
    uint8_t childPuppetBonesNumLayer1;
    const RawPuppetBoneRuntime *childPuppetBonesLayer2;
    uint8_t childPuppetBonesNumLayer2;
};

typedef struct
{
    const RawPuppetBoneRuntime *rawBone;
    const PuppetBoneAnimTimelineRuntime *rawAnimation;
} RawBoneAnimationPairRuntime;

typedef struct
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    const RawPuppetBoneRuntime *puppetBones;
    uint8_t puppetBonesNum;
    const RawBoneAnimationPairRuntime *boneAnimationPairs;
    uint8_t boneAnimationPairsNum;
} RawPuppetRuntime;

_Static_assert(sizeof(SpriteRuntime) == sizeof(Sprite), "Sprite runtime type must match Sprite layout");
_Static_assert(offsetof(SpriteRuntime, pixels) == offsetof(Sprite, pixels), "Sprite runtime pixels offset must match");
_Static_assert(offsetof(SpriteRuntime, height) == offsetof(Sprite, height), "Sprite runtime height offset must match");
_Static_assert(offsetof(SpriteRuntime, width) == offsetof(Sprite, width), "Sprite runtime width offset must match");
_Static_assert(offsetof(SpriteRuntime, canRotate) == offsetof(Sprite, canRotate), "Sprite runtime canRotate offset must match");
_Static_assert(sizeof(FontRuntime) == sizeof(Font), "Font runtime type must match Font layout");
_Static_assert(sizeof(ImageRuntime) == sizeof(Image), "Image runtime type must match Image layout");
_Static_assert(sizeof(ScrollerRuntime) == sizeof(Scroller), "Scroller runtime type must match Scroller layout");
_Static_assert(sizeof(KeyFrameRuntime) == sizeof(RawFrame), "KeyFrame runtime type must match RawFrame layout");
_Static_assert(offsetof(KeyFrameRuntime, x) == offsetof(RawFrame, x), "KeyFrame runtime x offset must match");
_Static_assert(offsetof(KeyFrameRuntime, y) == offsetof(RawFrame, y), "KeyFrame runtime y offset must match");
_Static_assert(offsetof(KeyFrameRuntime, angle) == offsetof(RawFrame, angle), "KeyFrame runtime angle offset must match");
_Static_assert(offsetof(KeyFrameRuntime, startFrameNum) == offsetof(RawFrame, startFrameNum), "KeyFrame runtime startFrameNum offset must match");
_Static_assert(sizeof(PuppetBoneAnimTimelineRuntime) == sizeof(RawAnimation), "PuppetBoneAnimTimeline runtime type must match RawAnimation layout");
_Static_assert(offsetof(PuppetBoneAnimTimelineRuntime, keyFrames) == offsetof(RawAnimation, frames), "PuppetBoneAnimTimeline runtime keyFrames offset must match");
_Static_assert(offsetof(PuppetBoneAnimTimelineRuntime, keyFramesNum) == offsetof(RawAnimation, framesNum), "PuppetBoneAnimTimeline runtime keyFramesNum offset must match");
_Static_assert(sizeof(RawPuppetBoneRuntime) == sizeof(RawPuppetBone), "RawPuppetBone runtime type must match RawPuppetBone layout");
_Static_assert(sizeof(RawBoneAnimationPairRuntime) == sizeof(RawBoneAnimationPair), "RawBoneAnimationPair runtime type must match RawBoneAnimationPair layout");
_Static_assert(offsetof(RawBoneAnimationPairRuntime, rawBone) == offsetof(RawBoneAnimationPair, rawBone), "RawBoneAnimationPair runtime rawBone offset must match");
_Static_assert(offsetof(RawBoneAnimationPairRuntime, rawAnimation) == offsetof(RawBoneAnimationPair, rawAnimation), "RawBoneAnimationPair runtime rawAnimation offset must match");
_Static_assert(sizeof(RawPuppetRuntime) == sizeof(RawPuppet), "RawPuppet runtime type must match RawPuppet layout");
_Static_assert(offsetof(RawPuppetRuntime, puppetBones) == offsetof(RawPuppet, puppetBones), "RawPuppet runtime puppetBones offset must match");
_Static_assert(offsetof(RawPuppetRuntime, puppetBonesNum) == offsetof(RawPuppet, puppetBonesNum), "RawPuppet runtime puppetBonesNum offset must match");
_Static_assert(offsetof(RawPuppetRuntime, boneAnimationPairs) == offsetof(RawPuppet, boneAnimationPairs), "RawPuppet runtime boneAnimationPairs offset must match");
_Static_assert(offsetof(RawPuppetRuntime, boneAnimationPairsNum) == offsetof(RawPuppet, boneAnimationPairsNum), "RawPuppet runtime boneAnimationPairsNum offset must match");

static const size_t fonts_count = sizeof(fonts) / sizeof(fonts[0]);
static const size_t images_count = sizeof(images) / sizeof(images[0]);
static const size_t models_count = sizeof(models) / sizeof(models[0]);
static const size_t effects_count = sizeof(effects) / sizeof(effects[0]);
static const size_t raw_puppets_count = sizeof(rawPuppets) / sizeof(rawPuppets[0]);
static const size_t scrollers_count = sizeof(scrollers) / sizeof(scrollers[0]);
static const size_t sprite_sheet_count = sizeof(spriteSheet) / sizeof(spriteSheet[0]);
static const size_t sprite_fonts_count = sizeof(spritesFonts) / sizeof(spritesFonts[0]);

static const Font *fonts_runtime = fonts;
static const Image *images_runtime = images;
static const Model *models_runtime = models;
static const uint32_t *const *effects_runtime = effects;
static const RawPuppet *raw_puppets_runtime = rawPuppets;
static const Scroller *scrollers_runtime = scrollers;
static const Sprite *sprite_sheet_runtime = spriteSheet;
static bool storage_initialized = false;

#if EUZEBIA3D_PSRAM_RUNTIME_ENABLED
typedef struct
{
    uint8_t *base;
    size_t capacity;
    size_t offset;
} PsramArena;

static size_t align_up(size_t value, size_t alignment)
{
    return (value + (alignment - 1u)) & ~(alignment - 1u);
}

static void *psram_alloc(PsramArena *arena, size_t size, size_t alignment)
{
    size_t aligned_offset;
    void *result;

    if (alignment == 0u)
        alignment = 1u;
    aligned_offset = align_up(arena->offset, alignment);
    if (aligned_offset > arena->capacity)
        return NULL;
    if (size > (arena->capacity - aligned_offset))
        return NULL;

    result = arena->base + aligned_offset;
    arena->offset = aligned_offset + size;
    return result;
}

static void *psram_copy(PsramArena *arena, const void *source, size_t size, size_t alignment)
{
    void *target = psram_alloc(arena, size, alignment);
    if (target == NULL)
        return NULL;
    memcpy(target, source, size);
    return target;
}

static bool init_psram_arena(PsramArena *arena)
{
    flash_devinfo_size_t psram_size_code;
    uint32_t psram_size_bytes;

    psram_size_code = flash_devinfo_get_cs_size(1);
    psram_size_bytes = flash_devinfo_size_to_bytes(psram_size_code);
    if (psram_size_bytes == 0u)
        return false;

    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
    arena->base = (uint8_t *)0x11000000u;
    arena->capacity = (size_t)psram_size_bytes;
    arena->offset = 0u;
    return true;
}

static const uint16_t *copy_u16_elements(PsramArena *arena, const uint16_t *source, size_t elements)
{
    return (const uint16_t *)psram_copy(arena, source, elements * sizeof(uint16_t), _Alignof(uint16_t));
}

static const Sprite *copy_sprites_with_pixels(PsramArena *arena, const Sprite *source, size_t count)
{
    size_t i;
    SpriteRuntime *sprites_copy;

    if (count == 0u)
        return source;

    sprites_copy = (SpriteRuntime *)psram_alloc(arena, count * sizeof(SpriteRuntime), _Alignof(SpriteRuntime));
    if (sprites_copy == NULL)
        return source;

    for (i = 0u; i < count; i++)
    {
        const uint16_t *pixels = source[i].pixels;
        size_t pixels_count = (size_t)source[i].width * (size_t)source[i].height;

        if ((pixels != NULL) && (pixels_count > 0u))
        {
            const uint16_t *pixels_copy = copy_u16_elements(arena, pixels, pixels_count);
            if (pixels_copy == NULL)
                return source;
            pixels = pixels_copy;
        }

        sprites_copy[i].pixels = pixels;
        sprites_copy[i].height = source[i].height;
        sprites_copy[i].width = source[i].width;
        sprites_copy[i].canRotate = source[i].canRotate;
    }

    return (const Sprite *)sprites_copy;
}

static const Font *copy_fonts_with_sprites(PsramArena *arena, const Sprite *font_sprites)
{
    size_t i;
    FontRuntime *fonts_copy;

    if (fonts_count == 0u)
        return fonts;

    fonts_copy = (FontRuntime *)psram_alloc(arena, fonts_count * sizeof(FontRuntime), _Alignof(FontRuntime));
    if (fonts_copy == NULL)
        return fonts;

    for (i = 0u; i < fonts_count; i++)
    {
        const Sprite *sprite_ref = fonts[i].sprite;

        if (sprite_ref != NULL)
        {
            ptrdiff_t sprite_index = sprite_ref - spritesFonts;
            if ((sprite_index >= 0) && ((size_t)sprite_index < sprite_fonts_count))
                sprite_ref = &font_sprites[sprite_index];
        }

        fonts_copy[i].width = fonts[i].width;
        fonts_copy[i].sprite = sprite_ref;
    }

    return (const Font *)fonts_copy;
}

static const Image *copy_images(PsramArena *arena)
{
    size_t i;
    ImageRuntime *images_copy;

    if (images_count == 0u)
        return images;

    images_copy = (ImageRuntime *)psram_alloc(arena, images_count * sizeof(ImageRuntime), _Alignof(ImageRuntime));
    if (images_copy == NULL)
        return images;

    for (i = 0u; i < images_count; i++)
    {
        const uint16_t *image_ptr = images[i].image;

        if ((image_ptr != NULL) && (images[i].size > 0u))
        {
            const uint16_t *image_copy = (const uint16_t *)psram_copy(arena, image_ptr, images[i].size, _Alignof(uint16_t));
            if (image_copy == NULL)
                return images;
            image_ptr = image_copy;
        }

        images_copy[i].image = image_ptr;
        images_copy[i].size = images[i].size;
        images_copy[i].width = images[i].width;
        images_copy[i].heigth = images[i].heigth;
    }

    return (const Image *)images_copy;
}

static const Scroller *copy_scrollers(PsramArena *arena)
{
    size_t i;
    ScrollerRuntime *scrollers_copy;

    if (scrollers_count == 0u)
        return scrollers;

    scrollers_copy = (ScrollerRuntime *)psram_alloc(arena, scrollers_count * sizeof(ScrollerRuntime), _Alignof(ScrollerRuntime));
    if (scrollers_copy == NULL)
        return scrollers;

    for (i = 0u; i < scrollers_count; i++)
    {
        const uint16_t *bitmap_ptr = scrollers[i].bitmap;
        size_t bitmap_elements = (size_t)scrollers[i].width * (size_t)scrollers[i].height;

        if ((bitmap_ptr != NULL) && (bitmap_elements > 0u))
        {
            const uint16_t *bitmap_copy = copy_u16_elements(arena, bitmap_ptr, bitmap_elements);
            if (bitmap_copy == NULL)
                return scrollers;
            bitmap_ptr = bitmap_copy;
        }

        scrollers_copy[i].bitmap = bitmap_ptr;
        scrollers_copy[i].width = scrollers[i].width;
        scrollers_copy[i].height = scrollers[i].height;
    }

    return (const Scroller *)scrollers_copy;
}

static const RawPuppetBoneRuntime *copy_raw_PuppetBones_recursive(PsramArena *arena, const RawPuppetBone *source, uint8_t count)
{
    uint8_t i;
    RawPuppetBoneRuntime *PuppetBones_copy;

    if ((source == NULL) || (count == 0u))
        return NULL;

    PuppetBones_copy = (RawPuppetBoneRuntime *)psram_alloc(arena, (size_t)count * sizeof(RawPuppetBoneRuntime), _Alignof(RawPuppetBoneRuntime));
    if (PuppetBones_copy == NULL)
        return NULL;

    for (i = 0u; i < count; i++)
    {
        const RawPuppetBoneRuntime *child_layer1 = NULL;
        const RawPuppetBoneRuntime *child_layer2 = NULL;

        if ((source[i].childPuppetBonesNumLayer1 > 0u) && (source[i].childPuppetBonesLayer1 != NULL))
        {
            child_layer1 = copy_raw_PuppetBones_recursive(arena, source[i].childPuppetBonesLayer1, source[i].childPuppetBonesNumLayer1);
            if (child_layer1 == NULL)
                return NULL;
        }
        if ((source[i].childPuppetBonesNumLayer2 > 0u) && (source[i].childPuppetBonesLayer2 != NULL))
        {
            child_layer2 = copy_raw_PuppetBones_recursive(arena, source[i].childPuppetBonesLayer2, source[i].childPuppetBonesNumLayer2);
            if (child_layer2 == NULL)
                return NULL;
        }

        PuppetBones_copy[i].label = source[i].label;
        PuppetBones_copy[i].x = source[i].x;
        PuppetBones_copy[i].y = source[i].y;
        PuppetBones_copy[i].angle = source[i].angle;
        PuppetBones_copy[i].spriteIndex = source[i].spriteIndex;
        PuppetBones_copy[i].baseSpriteAngle = source[i].baseSpriteAngle;
        PuppetBones_copy[i].childPuppetBonesLayer1 = child_layer1;
        PuppetBones_copy[i].childPuppetBonesNumLayer1 = source[i].childPuppetBonesNumLayer1;
        PuppetBones_copy[i].childPuppetBonesLayer2 = child_layer2;
        PuppetBones_copy[i].childPuppetBonesNumLayer2 = source[i].childPuppetBonesNumLayer2;
    }

    return PuppetBones_copy;
}

static const RawPuppetBoneRuntime *find_copied_raw_puppet_bone(const RawPuppetBone *source,
                                                               const RawPuppetBoneRuntime *copy,
                                                               uint8_t count,
                                                               const RawPuppetBone *target)
{
    uint8_t i;

    if ((source == NULL) || (copy == NULL) || (target == NULL))
        return NULL;

    for (i = 0u; i < count; i++)
    {
        const RawPuppetBoneRuntime *found;

        if (&source[i] == target)
            return &copy[i];

        found = find_copied_raw_puppet_bone(source[i].childPuppetBonesLayer1,
                                            copy[i].childPuppetBonesLayer1,
                                            source[i].childPuppetBonesNumLayer1,
                                            target);
        if (found != NULL)
            return found;

        found = find_copied_raw_puppet_bone(source[i].childPuppetBonesLayer2,
                                            copy[i].childPuppetBonesLayer2,
                                            source[i].childPuppetBonesNumLayer2,
                                            target);
        if (found != NULL)
            return found;
    }

    return NULL;
}

static const KeyFrameRuntime *copy_key_frames(PsramArena *arena, const RawFrame *source, uint16_t count)
{
    uint16_t i;
    KeyFrameRuntime *frames_copy;

    if ((source == NULL) || (count == 0u))
        return NULL;

    frames_copy = (KeyFrameRuntime *)psram_alloc(arena, (size_t)count * sizeof(KeyFrameRuntime), _Alignof(KeyFrameRuntime));
    if (frames_copy == NULL)
        return NULL;

    for (i = 0u; i < count; i++)
    {
        frames_copy[i].x = source[i].x;
        frames_copy[i].y = source[i].y;
        frames_copy[i].angle = source[i].angle;
        frames_copy[i].startFrameNum = source[i].startFrameNum;
    }

    return frames_copy;
}

static const PuppetBoneAnimTimelineRuntime *copy_puppet_bone_anim_timeline(PsramArena *arena, const RawAnimation *source)
{
    const KeyFrameRuntime *frames_ptr = NULL;
    PuppetBoneAnimTimelineRuntime *timeline_copy;

    if (source == NULL)
        return NULL;

    timeline_copy = (PuppetBoneAnimTimelineRuntime *)psram_alloc(arena, sizeof(PuppetBoneAnimTimelineRuntime), _Alignof(PuppetBoneAnimTimelineRuntime));
    if (timeline_copy == NULL)
        return NULL;

    if ((source->framesNum > 0u) && (source->frames != NULL))
    {
        frames_ptr = copy_key_frames(arena, source->frames, source->framesNum);
        if (frames_ptr == NULL)
            return NULL;
    }

    timeline_copy->keyFrames = frames_ptr;
    timeline_copy->keyFramesNum = source->framesNum;
    return timeline_copy;
}

static const RawBoneAnimationPairRuntime *copy_raw_bone_animation_pairs(PsramArena *arena,
                                                                        const RawPuppetBone *source_bones,
                                                                        const RawPuppetBoneRuntime *copied_bones,
                                                                        uint8_t source_bones_count,
                                                                        const RawBoneAnimationPair *source,
                                                                        uint8_t count)
{
    uint8_t i;
    RawBoneAnimationPairRuntime *pairs_copy;

    if ((source == NULL) || (count == 0u))
        return NULL;

    pairs_copy = (RawBoneAnimationPairRuntime *)psram_alloc(arena, (size_t)count * sizeof(RawBoneAnimationPairRuntime), _Alignof(RawBoneAnimationPairRuntime));
    if (pairs_copy == NULL)
        return NULL;

    for (i = 0u; i < count; i++)
    {
        const RawPuppetBoneRuntime *raw_bone = NULL;
        const PuppetBoneAnimTimelineRuntime *raw_animation = NULL;

        if (source[i].rawBone != NULL)
        {
            raw_bone = find_copied_raw_puppet_bone(source_bones, copied_bones, source_bones_count, source[i].rawBone);
            if (raw_bone == NULL)
                return NULL;
        }

        if (source[i].rawAnimation != NULL)
        {
            raw_animation = copy_puppet_bone_anim_timeline(arena, source[i].rawAnimation);
            if (raw_animation == NULL)
                return NULL;
        }

        pairs_copy[i].rawBone = raw_bone;
        pairs_copy[i].rawAnimation = raw_animation;
    }

    return pairs_copy;
}

static const RawPuppet *copy_raw_puppets(PsramArena *arena)
{
    size_t i;
    RawPuppetRuntime *puppets_copy;

    if (raw_puppets_count == 0u)
        return rawPuppets;

    puppets_copy = (RawPuppetRuntime *)psram_alloc(arena, raw_puppets_count * sizeof(RawPuppetRuntime), _Alignof(RawPuppetRuntime));
    if (puppets_copy == NULL)
        return rawPuppets;

    for (i = 0u; i < raw_puppets_count; i++)
    {
        const RawPuppetBoneRuntime *PuppetBones_ptr = NULL;
        const RawBoneAnimationPairRuntime *bone_animation_pairs_ptr = NULL;

        if ((rawPuppets[i].puppetBonesNum > 0u) && (rawPuppets[i].puppetBones != NULL))
        {
            PuppetBones_ptr = copy_raw_PuppetBones_recursive(arena, rawPuppets[i].puppetBones, rawPuppets[i].puppetBonesNum);
            if (PuppetBones_ptr == NULL)
                return rawPuppets;
        }
        if ((rawPuppets[i].boneAnimationPairsNum > 0u) && (rawPuppets[i].boneAnimationPairs != NULL))
        {
            bone_animation_pairs_ptr = copy_raw_bone_animation_pairs(arena,
                                                                     rawPuppets[i].puppetBones,
                                                                     PuppetBones_ptr,
                                                                     rawPuppets[i].puppetBonesNum,
                                                                     rawPuppets[i].boneAnimationPairs,
                                                                     rawPuppets[i].boneAnimationPairsNum);
            if (bone_animation_pairs_ptr == NULL)
                return rawPuppets;
        }

        puppets_copy[i].label = rawPuppets[i].label;
        puppets_copy[i].x = rawPuppets[i].x;
        puppets_copy[i].y = rawPuppets[i].y;
        puppets_copy[i].angle = rawPuppets[i].angle;
        puppets_copy[i].puppetBones = PuppetBones_ptr;
        puppets_copy[i].puppetBonesNum = rawPuppets[i].puppetBonesNum;
        puppets_copy[i].boneAnimationPairs = bone_animation_pairs_ptr;
        puppets_copy[i].boneAnimationPairsNum = rawPuppets[i].boneAnimationPairsNum;
    }

    return (const RawPuppet *)puppets_copy;
}

static const uint32_t *const *copy_effects(PsramArena *arena)
{
    size_t i;
    const uint32_t **effects_copy;
    const uint32_t *barrel_copy = barrel_distortion;

    if (effects_count == 0u)
        return effects;

    if (sizeof(barrel_distortion) > 0u)
    {
        const uint32_t *table_copy = (const uint32_t *)psram_copy(arena, barrel_distortion, sizeof(barrel_distortion), _Alignof(uint32_t));
        if (table_copy != NULL)
            barrel_copy = table_copy;
    }

    effects_copy = (const uint32_t **)psram_alloc(arena, effects_count * sizeof(*effects_copy), _Alignof(const uint32_t *));
    if (effects_copy == NULL)
        return effects;

    for (i = 0u; i < effects_count; i++)
    {
        const uint32_t *table = effects[i];
        if (table == barrel_distortion)
            table = barrel_copy;
        effects_copy[i] = table;
    }

    return (const uint32_t *const *)effects_copy;
}

static void copy_lookup_tables(PsramArena *arena)
{
    const int16_t *sin_copy;
    const int16_t *cos_copy;
    const int16_t *atan_copy;

    sin_copy = (const int16_t *)psram_copy(arena, get_sin_source(), EUZEBIA3D_SIN_TABLE_LENGTH * sizeof(int16_t), _Alignof(int16_t));
    cos_copy = (const int16_t *)psram_copy(arena, get_cos_source(), EUZEBIA3D_COS_TABLE_LENGTH * sizeof(int16_t), _Alignof(int16_t));
    atan_copy = (const int16_t *)psram_copy(arena, get_atan_source(), EUZEBIA3D_ATAN_TABLE_LENGTH * sizeof(int16_t), _Alignof(int16_t));
    if ((sin_copy == NULL) || (cos_copy == NULL) || (atan_copy == NULL))
        return;

    set_lookup_table_sources(sin_copy, cos_copy, atan_copy);
}

static void init_optional_psram_assets(void)
{
    PsramArena arena;
    const Sprite *font_sprites_psram;

    if (!init_psram_arena(&arena))
        return;

    font_sprites_psram = copy_sprites_with_pixels(&arena, spritesFonts, sprite_fonts_count);
    if (font_sprites_psram != spritesFonts)
    {
        const Font *fonts_psram = copy_fonts_with_sprites(&arena, font_sprites_psram);
        if (fonts_psram != fonts)
            fonts_runtime = fonts_psram;
    }

    {
        const Sprite *sprites_psram = copy_sprites_with_pixels(&arena, spriteSheet, sprite_sheet_count);
        if (sprites_psram != spriteSheet)
            sprite_sheet_runtime = sprites_psram;
    }

    {
        const Image *images_psram = copy_images(&arena);
        if (images_psram != images)
            images_runtime = images_psram;
    }

    {
        const uint32_t *const *effects_psram = copy_effects(&arena);
        if (effects_psram != effects)
            effects_runtime = effects_psram;
    }

    {
        const Scroller *scrollers_psram = copy_scrollers(&arena);
        if (scrollers_psram != scrollers)
            scrollers_runtime = scrollers_psram;
    }

    {
        const RawPuppet *puppets_psram = copy_raw_puppets(&arena);
        if (puppets_psram != rawPuppets)
            raw_puppets_runtime = puppets_psram;
    }

    copy_lookup_tables(&arena);
}
#endif

static void ensure_storage_initialized(void)
{
    if (storage_initialized)
        return;

    storage_initialized = true;
    reset_lookup_table_sources();
#if EUZEBIA3D_PSRAM_RUNTIME_ENABLED
    init_optional_psram_assets();
#endif
}

const Font *get_font_by_index(uint8_t index)
{
    return &fonts_runtime[index];
}

const Image *get_image(uint8_t image_index)
{
    return &images_runtime[image_index];
}

const Model *get_model(uint8_t model_index)
{
    (void)models_count;
    return &models_runtime[model_index];
}

const uint32_t *get_effect_table(uint8_t effect_index)
{
    return effects_runtime[effect_index];
}

const uint32_t get_effect_table_element(uint8_t effect_index, uint32_t e_index)
{
    return effects_runtime[effect_index][e_index];
}

const RawPuppet *get_raw_puppet(uint8_t puppetIndex)
{
    if (puppetIndex >= raw_puppets_count)
        return NULL;
    return &raw_puppets_runtime[puppetIndex];
}

const Scroller *get_scroller_by_index(uint8_t index)
{
    if (index >= scrollers_count)
        return NULL;
    return &scrollers_runtime[index];
}

const Sprite *get_sprite(uint8_t sprite_index)
{
    if (sprite_index >= sprite_sheet_count)
        return NULL;
    return &sprite_sheet_runtime[sprite_index];
}

static IStorage storage = {
    .get_font_by_index = get_font_by_index,
    .get_image = get_image,
    .get_model = get_model,
    .get_effect_table = get_effect_table,
    .get_effect_table_element = get_effect_table_element,
    .get_raw_puppet = get_raw_puppet,
    .get_scroller_by_index = get_scroller_by_index,
    .get_sprite = get_sprite,
};

const IStorage *get_storage(void)
{
    ensure_storage_initialized();
    return &storage;
}
