#include "display.h"
#include "IDisplay.h"

static e3d_IDisplay display = {
    .init_display = display_init,
    .present_framebuffer = display_present_framebuffer,
};

const e3d_IDisplay *get_display(void) { return &display; }
