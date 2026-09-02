#ifndef IDISPLAY_h
#define IDISPLAY_h

#include "IHardware.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  void (*init_display)(const e3d_IHardware *hardware);
  bool (*present_framebuffer)(const uint16_t *framebuffer,
                              volatile uint32_t *debug_stage,
                              volatile uint32_t *debug_line);
} e3d_IDisplay;

#endif
