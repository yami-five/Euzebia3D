#ifndef IDISPLAY_h
#define IDISPLAY_h

#include "IHardware.h"
#include <stdint.h>

typedef struct {
  void (*init_display)(const IHardware *hardware);
} IDisplay;

#endif