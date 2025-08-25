#include "stdint.h"

typedef struct {
    const uint16_t *bitmap;
    const uint8_t width;
    const uint8_t height;
} Scroller;

const Scroller *get_scroller_by_index(uint8_t index);