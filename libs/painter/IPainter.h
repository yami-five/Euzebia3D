#ifndef IPAINTER_h
#define IPAINTER_h

#include "IDisplay.h"
#include "IHardware.h"
#include "IStorage.h"
#include "painter_types.h"
#include "puppet.h"
#include <stdint.h>

typedef struct {
  void (*init_painter)(const e3d_IDisplay *display, const e3d_IHardware *hardware,
                       const e3d_IStorage *storage);
  void (*draw_buffer)(void);
  void (*clear_buffer)(uint16_t color);
  void (*draw_pixel)(uint16_t x, uint16_t y, uint16_t color);
  void (*draw_span)(uint16_t x, uint16_t y, const uint16_t *span,
                    uint16_t span_length);
  void (*draw_image)(uint8_t image_index);
  void (*apply_post_process_effect)(uint8_t effect_index, uint16_t *params);
  void (*draw_sprite)(const e3d_Sprite *sprite, int16_t pos_x, int16_t pos_y,
                      int32_t angle, uint8_t scale);
  void (*draw_puppet)(e3d_Puppet *puppet);
  void (*draw_background)(e3d_Image *image);
  void (*print)(const char *text, int16_t x, int16_t y, uint8_t fontIndex,
                uint16_t color);
  void (*fade_fullscreen)(uint8_t mode, uint32_t startFrame,
                          uint32_t currentFrame);
  void (*draw_scroller)(const e3d_Scroller *scroller, uint16_t x, uint16_t y,
                        uint32_t startFrame, uint32_t currentFrame);
  void (*draw_plasma)(uint16_t *colors, uint16_t colorsNum, uint32_t t,
                      uint8_t scale, int8_t facA, int8_t facB, int8_t facC,
                      int8_t facD, e3d_Rectangle *rectangle);
  void (*draw_rectangle)(e3d_Rectangle *rect, uint16_t color);
  void (*draw_line)(e3d_Point *start, e3d_Point *end, uint16_t color);
  void (*draw_gradient)(uint16_t colorA, uint16_t colorB, e3d_Rectangle *rectangle,
                        uint8_t direction);
} e3d_IPainter;

#endif
