#ifndef PAINTER_PLATFORM_H
#define PAINTER_PLATFORM_H

#include "IPainter.h"
#include <stdbool.h>

void painter_platform_init(const e3d_IDisplay *display,
                           const e3d_IHardware *hardware);
bool painter_platform_draw_buffer(const uint16_t *buffer,
                                  volatile uint32_t *debug_stage,
                                  volatile uint32_t *debug_line);
void painter_platform_draw_image(uint16_t *buffer, const e3d_Image *image);

#endif
