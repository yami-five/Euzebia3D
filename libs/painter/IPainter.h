#ifndef IPAINTER_h
#define IPAINTER_h

#include <stdint.h>
#include "IDisplay.h"
#include "IHardware.h"
#include "puppet.h"
#include "../storage/sprites.h"
#include "gradient.h"
#include "../storage/scrollers.h"

typedef struct
{
    void (*init_painter)(const IDisplay *display, const IHardware *hardware);
    void (*draw_buffer)(void);
    void (*clear_buffer)(uint16_t color);
    void (*draw_pixel)(uint16_t x, uint16_t y, uint16_t color);
    void (*draw_image)(uint8_t image_index);
    void (*apply_post_process_effect)(uint8_t effect_index, uint16_t *params);
    void (*draw_sprite)(const Sprite *sprite, int16_t pos_x, int16_t pos_y, int32_t angle, uint8_t scale);
    void (*draw_puppet)(Puppet *puppet);
    void (*print)(const char *text, int16_t x, int16_t y, uint8_t scale);
    void (*draw_gradient)(Gradient *gradient);
    void (*override_buffer)(uint8_t mode, uint16_t lines);
    void (*fade_fullscreen)(uint8_t mode, uint32_t startFrame, uint32_t currentFrame);
    void (*draw_scroller)(const Scroller* scroller, uint16_t x, uint16_t y, uint32_t startFrame, uint32_t currentFrame);
    void (*fade)(uint8_t mode, uint32_t startFrame, uint32_t currentFrame, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
} IPainter;

#endif