#include "gradient.h"

static GradientColor sunrise1 = {
    .r = 0,
    .g = 0,
    .b = 0,
    .pos = 0,
};

static GradientColor sunrise2 = {
    .r = 66,
    .g = 5,
    .b = 45,
    .pos = 350,
};

static GradientColor sunrise3 = {
    .r = 107,
    .g = 8,
    .b = 72,
    .pos = 430,
};

static GradientColor sunrise4 = {
    .r = 163,
    .g = 10,
    .b = 59,
    .pos = 500,
};

static GradientColor sunrise5 = {
    .r = 236,
    .g = 97,
    .b = 10,
    .pos = 560,
};

static GradientColor sunrise6 = {
    .r = 255,
    .g = 193,
    .b = 0,
    .pos = 610,
};

static GradientColor sunrise7 = {
    .r = 255,
    .g = 255,
    .b = 255,
    .pos = 640,
};

static GradientColor *sunriseColors[7] = {
    &sunrise1,
    &sunrise2,
    &sunrise3,
    &sunrise4,
    &sunrise5,
    &sunrise6,
    &sunrise7,
};

static Gradient sunrise = {
    .colors = sunriseColors,
    .colorsNum = 7,
};

static Gradient *gradients[1] = {
    &sunrise,
};

Gradient *get_gradient_by_index(uint8_t index)
{
    return gradients[index];
}

void move_gradient(Gradient *gradient, uint8_t offset)
{
    GradientColor **colors = gradient->colors;
    for (uint8_t i = 0; i < gradient->colorsNum; i++)
    {
        if(gradient->colors[i]->pos>0)
            gradient->colors[i]->pos -= offset;
    }
}