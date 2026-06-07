#ifndef IDISPLAY_h
#define IDISPLAY_h

#include <stdint.h>
#include "IHardware.h"

typedef struct
{
    void (*init_display)(const IHardware* hardware);
} IDisplay;

#endif