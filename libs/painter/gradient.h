#ifndef GRADIENT_H
#define GRADIENT_H

#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint16_t pos;
} GradientColor;

typedef struct
{
    uint8_t colorsNum;
    GradientColor **colors;
} Gradient;

Gradient *get_gradient_by_index(uint8_t index);
void move_gradient(Gradient *gradient, uint8_t offset);

#endif