#ifndef IPUPPETFACTORY_h
#define IPUPPETFACTORY_h

#include "IStorage.h"
#include "puppet.h"
#include <stdint.h>

typedef struct {
  void (*init_puppet_factory)(const IStorage *storage);
  Puppet *(*create)(uint8_t puppetIndex);
} IPuppetFactory;

#endif