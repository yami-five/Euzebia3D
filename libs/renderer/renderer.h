#ifndef RENDERER_h
#define RENDERER_h

#include "IRenderer.h"
#include "fpa.h"

const IRenderer *get_renderer(void);
void renderer_set_scale(uint8_t scale);

#endif
