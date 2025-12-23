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
    void (*add_model_to_scene)(Mesh *mesh, Camera *camera, PointLight *pLight);
    void (*clean_scene)();
    void (*render_scene)(PointLight *pLight);
    void (*set_scale)(uint8_t scale);
} IRenderer;

#endif
