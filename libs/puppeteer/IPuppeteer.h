#ifndef IPUPPETEER_h
#define IPUPPETEER_h

#include "IPainter.h"
#include "IStorage.h"
#include <stdint.h>

typedef struct {
  void (*init_puppeteer)(const e3d_IStorage *storage, const e3d_IPainter *painter);
  e3d_Puppet *(*create_puppet)(uint8_t puppetIndex);
  void (*perform)(e3d_Puppet *puppet, uint32_t t);
} e3d_IPuppeteer;

#endif