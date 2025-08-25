#ifndef IRENDERER_h
#define IRENDERER_h

#include <stdint.h>
#include "IPainter.h"
#include "IHardware.h"
#include "mesh.h"
#include "light.h"
#include "camera.h"

typedef struct
{
    void (*init_renderer)(volatile const IHardware * hardware, volatile const IPainter * painter);
    void (*draw_model)();
    void (*clear_zbuffer)(void);
} IRenderer;

#endif