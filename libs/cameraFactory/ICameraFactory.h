#ifndef ICAMERAFACTORY_h
#define ICAMERAFACTORY_h

#include "camera.h"
#include <stdint.h>

typedef struct {
  e3d_Camera *(*create_camera)(float camX, float camY, float camZ, float targetX,
                           float targetY, float targetZ, float upX, float upY,
                           float upZ);
} e3d_ICameraFactory;

#endif
