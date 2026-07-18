#ifndef IRENDERER_h
#define IRENDERER_h

#include "IHardware.h"
#include "IPainter.h"
#include "camera.h"
#include "light.h"
#include "mesh.h"
#include "primitives.h"
#include <stdint.h>

typedef struct {
  void (*init_renderer)(const IHardware *hardware, const IPainter *painter);
  void (*add_model_to_scene)(Mesh *mesh);
  void (*add_point_to_scene)(Point3D *point);
  void (*add_line_to_scene)(Line3D *line);
  void (*clean_scene)();
  void (*render_scene)();
  void (*set_scale)(uint8_t scale);
  void (*set_camera)(Camera *camera);
  void (*set_light)(Light *light);
} IRenderer;

#endif
