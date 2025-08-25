#include <stdint.h>

typedef struct
{
    int x;
    int y;
    float angle;
} Frame;

typedef struct
{
    const char* label;
    const Frame* frames;
    const uint16_t framesNum;
} Animation;

const Animation* get_animation_by_index(uint8_t index);