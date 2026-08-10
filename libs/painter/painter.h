#ifndef PAINTER_h
#define PAINTER_h

#include "IPainter.h"
#include "../storage/gfx.h"
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <stdlib.h>
#else
#include "../storage/pins.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#endif
#include "stdio.h"
#include "string.h"

#define WIDTH_DOUBLED 640
#define HEIGHT_DOUBLED 480
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define WIDTH_HALF 160
#define HEIGHT_HALF 120
#define BUFFER_SIZE 153600
#define BUFFER_SIZE_HALF 76800

const e3d_IPainter *get_painter(void);

#endif