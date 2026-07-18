#ifndef IRENDERER_h
#define IRENDERER_h

#include <stdint.h>
#include "IPainter.h"
#include "IHardware.h"
#include "mesh.h"
#include "light.h"
#include "camera.h"
#include "primitives.h"

typedef struct
{
    void (*init_renderer)(const IHardware * hardware, const IPainter * painter);
    void (*add_model_to_scene)(Mesh *mesh, Camera *camera, Light *light);
    void (*add_point_to_scene)(Point3D *point, Camera *camera);
    void (*add_line_to_scene)(Line3D *line, Camera *camera);
    void (*clean_scene)();
    void (*render_scene)(Light *light);
    void (*set_scale)(uint8_t scale);
} IRenderer;

#endif
