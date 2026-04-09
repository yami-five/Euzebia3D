#ifndef SPRITES_H
#define SPRITES_H

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef struct
{
    const uint16_t *pixels;
    const uint8_t size;
    const bool canRotate;
} Sprite;

#endif