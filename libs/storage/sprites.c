#include "sprites.h"

const Sprite spriteSheet[0] = {
    
};

const Sprite *get_sprite(uint8_t sprite_index)
{
    if (sprite_index < 255)
        return &spriteSheet[sprite_index];
    else
        return NULL;
}