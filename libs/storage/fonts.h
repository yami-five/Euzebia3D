#ifndef FONTS_H
#define FONTS_H

#include "sprites.h"
#include "stdint.h"

typedef struct 
{
    const uint16_t *characters;
    const uint8_t size;
} Font;

#endif