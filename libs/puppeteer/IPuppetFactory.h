#ifndef IPUPPETFACTORY_h
#define IPUPPETFACTORY_h

#include "IStorage.h"
#include "puppet.h"
#include <stdint.h>

typedef struct {
  void (*init_puppet_factory)(const e3d_IStorage *storage);
  e3d_Puppet *(*create)(uint8_t puppetIndex);
} e3d_IPuppetFactory;

#endif