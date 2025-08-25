#ifndef IPUPPETFACTORY_h
#define IPUPPETFACTORY_h

#include <stdint.h>
#include "puppet.h"

typedef struct
{
    Puppet *(*create_puppet)(uint8_t puppetIndex);
} IPuppetFactory;

#endif