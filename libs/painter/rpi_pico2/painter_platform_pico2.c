#include "painter_platform.h"

#include "../../storage/pins.h"
#include <stddef.h>

static const e3d_IDisplay *painter_display = NULL;

void painter_platform_init(const e3d_IDisplay *display) {
  painter_display = display;
}

bool painter_platform_draw_buffer(const uint16_t *buffer,
                                  volatile uint32_t *debug_stage,
                                  volatile uint32_t *debug_line) {
  return painter_display->present_framebuffer(buffer, debug_stage, debug_line);
}
