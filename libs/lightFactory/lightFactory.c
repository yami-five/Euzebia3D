#include "ILightFactory.h"
#include "lightFactory.h"
#include "light.h"
#include "fpa.h"
#include "../shared/gfx.h"
#include <stdlib.h>

PointLight* create_point_light(float x, float y, float z, float intensity, uint16_t color)
{
    PointLight *light = (PointLight *)malloc(sizeof(PointLight));
    light->position.x = float_to_fixed(x);
    light->position.y = float_to_fixed(y);
    light->position.z = float_to_fixed(z);
    light->intensity = float_to_fixed(intensity);
    light->color = color;
    return light;
}

static ILightFactory light = {
    .create_point_light = create_point_light,
};

const ILightFactory *get_lightFactory(void)
{
    return &light;
}