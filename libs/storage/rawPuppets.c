#include "rawPuppets.h"
#include "string.h"


static const RawPuppet rawPuppets[0] = {
};

const RawPuppet *get_raw_puppet(uint8_t puppetIndex)
{
    return &rawPuppets[puppetIndex];
}