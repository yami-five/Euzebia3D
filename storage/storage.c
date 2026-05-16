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
    uint8_t size;
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

typedef struct RawBoneRuntime RawBoneRuntime;
struct RawBoneRuntime
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    uint8_t spriteIndex;
    float baseSpriteAngle;
    const RawBoneRuntime *childBonesLayer1;
    uint8_t childBonesNumLayer1;
    const RawBoneRuntime *childBonesLayer2;
    uint8_t childBonesNumLayer2;
};

typedef struct
{
    const char *label;
    int16_t x;
    int16_t y;
    float angle;
    const RawBoneRuntime *bones;
    uint8_t bonesNum;
} RawPuppetRuntime;

_Static_assert(sizeof(SpriteRuntime) == sizeof(Sprite), "Sprite runtime type must match Sprite layout");
_Static_assert(sizeof(FontRuntime) == sizeof(Font), "Font runtime type must match Font layout");
_Static_assert(sizeof(ImageRuntime) == sizeof(Image), "Image runtime type must match Image layout");
_Static_assert(sizeof(ScrollerRuntime) == sizeof(Scroller), "Scroller runtime type must match Scroller layout");
_Static_assert(sizeof(RawBoneRuntime) == sizeof(RawBone), "RawBone runtime type must match RawBone layout");
_Static_assert(sizeof(RawPuppetRuntime) == sizeof(RawPuppet), "RawPuppet runtime type must match RawPuppet layout");

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
        size_t pixels_count = (size_t)source[i].size * (size_t)source[i].size;

        if ((pixels != NULL) && (pixels_count > 0u))
        {
            const uint16_t *pixels_copy = copy_u16_elements(arena, pixels, pixels_count);
            if (pixels_copy == NULL)
                return source;
            pixels = pixels_copy;
        }

        sprites_copy[i].pixels = pixels;
        sprites_copy[i].size = source[i].size;
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

static const RawBoneRuntime *copy_raw_bones_recursive(PsramArena *arena, const RawBone *source, uint8_t count)
{
    uint8_t i;
    RawBoneRuntime *bones_copy;

    if ((source == NULL) || (count == 0u))
        return NULL;

    bones_copy = (RawBoneRuntime *)psram_alloc(arena, (size_t)count * sizeof(RawBoneRuntime), _Alignof(RawBoneRuntime));
    if (bones_copy == NULL)
        return NULL;

    for (i = 0u; i < count; i++)
    {
        const char *label_ptr = source[i].label;
        const RawBoneRuntime *child_layer1 = NULL;
        const RawBoneRuntime *child_layer2 = NULL;

        if (label_ptr != NULL)
        {
            size_t label_len = strlen(label_ptr) + 1u;
            char *label_copy = (char *)psram_copy(arena, label_ptr, label_len, _Alignof(char));
            if (label_copy == NULL)
                return NULL;
            label_ptr = label_copy;
        }

        if ((source[i].childBonesNumLayer1 > 0u) && (source[i].childBonesLayer1 != NULL))
        {
            child_layer1 = copy_raw_bones_recursive(arena, source[i].childBonesLayer1, source[i].childBonesNumLayer1);
            if (child_layer1 == NULL)
                return NULL;
        }
        if ((source[i].childBonesNumLayer2 > 0u) && (source[i].childBonesLayer2 != NULL))
        {
            child_layer2 = copy_raw_bones_recursive(arena, source[i].childBonesLayer2, source[i].childBonesNumLayer2);
            if (child_layer2 == NULL)
                return NULL;
        }

        bones_copy[i].label = label_ptr;
        bones_copy[i].x = source[i].x;
        bones_copy[i].y = source[i].y;
        bones_copy[i].angle = source[i].angle;
        bones_copy[i].spriteIndex = source[i].spriteIndex;
        bones_copy[i].baseSpriteAngle = source[i].baseSpriteAngle;
        bones_copy[i].childBonesLayer1 = child_layer1;
        bones_copy[i].childBonesNumLayer1 = source[i].childBonesNumLayer1;
        bones_copy[i].childBonesLayer2 = child_layer2;
        bones_copy[i].childBonesNumLayer2 = source[i].childBonesNumLayer2;
    }

    return bones_copy;
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
        const char *label_ptr = rawPuppets[i].label;
        const RawBoneRuntime *bones_ptr = NULL;

        if (label_ptr != NULL)
        {
            size_t label_len = strlen(label_ptr) + 1u;
            char *label_copy = (char *)psram_copy(arena, label_ptr, label_len, _Alignof(char));
            if (label_copy == NULL)
                return rawPuppets;
            label_ptr = label_copy;
        }

        if ((rawPuppets[i].bonesNum > 0u) && (rawPuppets[i].bones != NULL))
        {
            bones_ptr = copy_raw_bones_recursive(arena, rawPuppets[i].bones, rawPuppets[i].bonesNum);
            if (bones_ptr == NULL)
                return rawPuppets;
        }

        puppets_copy[i].label = label_ptr;
        puppets_copy[i].x = rawPuppets[i].x;
        puppets_copy[i].y = rawPuppets[i].y;
        puppets_copy[i].angle = rawPuppets[i].angle;
        puppets_copy[i].bones = bones_ptr;
        puppets_copy[i].bonesNum = rawPuppets[i].bonesNum;
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
    uint32_t i;
    int *sin_copy;
    int *cos_copy;
    int *atan_copy;

    sin_copy = (int *)psram_alloc(arena, EUZEBIA3D_SIN_TABLE_LENGTH * sizeof(int), _Alignof(int));
    cos_copy = (int *)psram_alloc(arena, EUZEBIA3D_COS_TABLE_LENGTH * sizeof(int), _Alignof(int));
    atan_copy = (int *)psram_alloc(arena, EUZEBIA3D_ATAN_TABLE_LENGTH * sizeof(int), _Alignof(int));
    if ((sin_copy == NULL) || (cos_copy == NULL) || (atan_copy == NULL))
        return;

    for (i = 0u; i < EUZEBIA3D_SIN_TABLE_LENGTH; i++)
        sin_copy[i] = (int)get_sin((uint16_t)i);
    for (i = 0u; i < EUZEBIA3D_COS_TABLE_LENGTH; i++)
        cos_copy[i] = (int)get_cos((uint16_t)i);
    for (i = 0u; i < EUZEBIA3D_ATAN_TABLE_LENGTH; i++)
        atan_copy[i] = (int)get_atan((uint16_t)i);

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
