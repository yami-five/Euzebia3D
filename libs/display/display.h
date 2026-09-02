#ifndef DISPLAY_h
#define DISPLAY_h

#include "IDisplay.h"

void display_init(const e3d_IHardware *hardware);
bool display_present_framebuffer(const uint16_t *framebuffer,
                                 volatile uint32_t *debug_stage,
                                 volatile uint32_t *debug_line);

const e3d_IDisplay *get_display(void);

#endif
