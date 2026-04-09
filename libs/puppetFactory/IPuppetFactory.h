#ifndef IPUPPETFACTORY_h
#define IPUPPETFACTORY_h

#include <stdint.h>
#include "IStorage.h"
#include "puppet.h"

typedef struct
{
    void (*init_puppet_factory)(volatile const IStorage * storage);
    Puppet *(*create_puppet)(uint8_t puppetIndex);
} IPuppetFactory;

#endif