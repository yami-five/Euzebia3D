#ifndef IPUPPETEER_h
#define IPUPPETEER_h

#include "IPainter.h"
#include "IStorage.h"
#include <stdint.h>

typedef struct {
  void (*init_puppeteer)(const IStorage *storage, const IPainter *painter);
  Puppet *(*create_puppet)(uint8_t puppetIndex);
  void (*perform)(Puppet *puppet, uint32_t t);
} IPuppeteer;

#endif