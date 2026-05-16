#ifndef DISPLAY_h
#define DISPLAY_h

#include "IDisplay.h"
#include "stdio.h"

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "pico/stdlib.h"
#endif

const IDisplay *get_display(void);

#endif
