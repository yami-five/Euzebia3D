#include "storage.h"

// Single translation unit for all storage implementation.
#include "fonts.c"
#include "gfx.c"
#include "post_processing.c"
#include "rawPuppets.c"
#include "scrollers.c"
#include "sprites.c"

const Font *get_font_by_index(uint8_t index)
{
    return &fonts[index];
}

const Image *get_image(uint8_t image_index)
{
    return &images[image_index];
}

const Model *get_model(uint8_t model_index)
{
    return &models[model_index];
}

const uint32_t *get_effect_table(uint8_t effect_index)
{
    return effects[effect_index];
}

const uint32_t get_effect_table_element(uint8_t effect_index, uint32_t e_index)
{
    return effects[effect_index][e_index];
}

const RawPuppet *get_raw_puppet(uint8_t puppetIndex)
{
    return &rawPuppets[puppetIndex];
}

const Scroller *get_scroller_by_index(uint8_t index)
{
    return &scrollers[index];
}

const Sprite *get_sprite(uint8_t sprite_index)
{
    if (sprite_index < 255)
        return &spriteSheet[sprite_index];
    else
        return NULL;
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
    return &storage;
}
