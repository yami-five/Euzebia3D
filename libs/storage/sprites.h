#ifndef SPRITES_H
#define SPRITES_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

typedef struct {
  const uint16_t *pixels;
  const uint16_t height;
  const uint16_t width;
  const bool canRotate;
} e3d_Sprite;

#endif