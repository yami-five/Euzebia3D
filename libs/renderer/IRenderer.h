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
  void (*init_renderer)(const e3d_IHardware *hardware, const e3d_IPainter *painter);
  void (*add_model_to_scene)(e3d_Mesh *mesh);
  void (*add_point_to_scene)(e3d_Point3D *point);
  void (*add_line_to_scene)(e3d_Line3D *line);
  void (*clean_scene)();
  void (*render_scene)();
  void (*set_scale)(uint8_t scale);
  void (*set_camera)(e3d_Camera *camera);
  void (*set_light)(e3d_Light *light);
  void (*unset_camera)(const e3d_Camera *camera);
  void (*unset_light)(const e3d_Light *light);
  void (*remove_material_from_scene)(const e3d_Material *material);
} e3d_IRenderer;

#endif
