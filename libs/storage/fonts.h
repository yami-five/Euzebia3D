#ifndef FONTS_H
#define FONTS_H

#include "sprites.h"
#include "stdint.h"

typedef struct 
{
    const Sprite *sprite;
    const uint8_t width;
} Font;

#endif