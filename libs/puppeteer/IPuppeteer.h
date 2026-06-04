#ifndef IPUPPETEER_h
#define IPUPPETEER_h

#include <stdint.h>
#include "IStorage.h"
#include "IPainter.h"

typedef struct
{
    void (*init_puppeteer)(const IStorage *storage, const IPainter *painter);
    Puppet *(*create_puppet)(uint8_t puppetIndex);
    void(*perform)(Puppet *puppet, uint8_t animationIndex, uint32_t t);
} IPuppeteer;

#endif