#ifndef PAINTER_PLATFORM_H
#define PAINTER_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include "IDisplay.h"

void painter_platform_init(const e3d_IDisplay *display);
bool painter_platform_draw_buffer(const uint16_t *buffer,
                                  volatile uint32_t *debug_stage,
                                  volatile uint32_t *debug_line);

#endif
