#ifndef IDISPLAY_h
#define IDISPLAY_h

#include <stdint.h>
#include "IHardware.h"

typedef struct
{
    void (*init_display)(volatile const IHardware* hardware);
} IDisplay;

#endif