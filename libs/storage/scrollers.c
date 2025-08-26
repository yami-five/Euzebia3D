#include "scrollers.h"

static const Scroller scrollers[0] = {
    
};

const Scroller *get_scroller_by_index(uint8_t index)
{
    return &scrollers[index];
}