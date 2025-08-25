#ifndef ICAMERAFACTORY_h
#define ICAMERAFACTORY_h

#include <stdint.h>
#include "camera.h"

typedef struct
{
    Camera* (*create_camera)(float camX, float camY, float camZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);
} ICameraFactory;

#endif