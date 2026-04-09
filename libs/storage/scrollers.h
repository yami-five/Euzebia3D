#ifndef SCROLLERS_H
#define SCROLLERS_H

#include "stdint.h"

typedef struct {
    const uint16_t *bitmap;
    const uint8_t width;
    const uint8_t height;
} Scroller;

#endif
