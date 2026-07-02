#ifndef PAINTER_TYPES_h
#define PAINTER_TYPES_h

#include <stdint.h>

typedef struct
{
    int16_t x;
    int16_t y;
} Point;

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t height;
    uint16_t width;
} Rectangle;

#define UP 0u
#define DOWN 1u
#define LEFT 2u
#define RIGHT 3u

#endif