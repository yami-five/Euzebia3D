#ifndef SPRITES_H
#define SPRITES_H

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef struct
{
    const uint16_t *pixels;
    const uint16_t height;
    const uint16_t width;
    const bool canRotate;
} Sprite;

#endif