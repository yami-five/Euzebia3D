#include "display.h"

void display_init(const e3d_IHardware *hardware) { (void)hardware; }

bool display_present_framebuffer(const uint16_t *framebuffer,
                                 volatile uint32_t *debug_stage,
                                 volatile uint32_t *debug_line) {
  (void)framebuffer;
  (void)debug_stage;
  (void)debug_line;
  return true;
}