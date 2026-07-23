#ifndef ISTORAGE_h
#define ISTORAGE_h

#include "fonts.h"
#include "gfx.h"
#include "post_processing.h"
#include "rawPuppets.h"
#include "scrollers.h"
#include "sprites.h"
#include "stdint.h"

typedef struct {
  const e3d_Font *(*get_font)(uint8_t index);
  const e3d_Image *(*get_image)(uint8_t image_index);
  const e3d_Model *(*get_model)(uint8_t model_index);
  const uint32_t *(*get_effect_table)(uint8_t effect_index);
  const uint32_t (*get_effect_table_element)(uint8_t effect_index,
                                             uint32_t e_index);
  const e3d_RawPuppet *(*get_raw_puppet)(uint8_t puppetIndex);
  const e3d_Scroller *(*get_scroller_by_index)(uint8_t index);
  const e3d_Sprite *(*get_sprite)(uint8_t sprite_index);
} e3d_IStorage;

#endif
