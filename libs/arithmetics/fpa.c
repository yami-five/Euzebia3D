#include "fpa.h"
#include "lookup_tables.h"
// int32_t sin_table[TABLE_SIZE];
// int32_t cos_table[TABLE_SIZE];

int32_t float_to_fixed(float value)
{
    return (int32_t)(value * SCALE_FACTOR);
}

int32_t fixed_to_float(int32_t value)
{
    return (float)(value>>SHIFT_FACTOR);
}

int32_t fixed_add(int32_t a, int32_t b)
{
    return a + b;
}

int32_t fixed_add3(int32_t a, int32_t b, int32_t c)
{
    return a + b + c;
}

int32_t fixed_sub(int32_t a, int32_t b)
{
    return a - b;
}

int64_t fixed_mul(int32_t a, int32_t b)
{
    int64_t result = (int64_t)a * b;
    return (int64_t)(result>>SHIFT_FACTOR);
}

int32_t fixed_div(int32_t a, int32_t b)
{
    int64_t result = ((int64_t)a<<SHIFT_FACTOR) / b;
    return (int32_t)result;
}

int64_t fixed_pow(int32_t a)
{
    return fixed_mul(a, a);
}

// void init_sin_cos()
// {
//     for (int32_t i = 0; i < TABLE_SIZE; i++)
//     {
//         float angle = i * RESOLUTION;
//         sin_table[i] = float_to_fixed(sinf(angle));
//         cos_table[i] = float_to_fixed(cosf(angle));
//     }
// }

int16_t fast_sin(int32_t value)
{
    int32_t index = value % TABLE_SIZE;
    if (index < 0)
        index += TABLE_SIZE;
    return get_sin(index);
}

int16_t fast_cos(int32_t value)
{
    int32_t index = value % TABLE_SIZE;
    if (index < 0)
        index += TABLE_SIZE;
    return get_cos(index);
}

int16_t fast_atan2(int16_t y, int16_t x)
{
    int32_t index = (y + 70) * 141 + x + 70;
    return get_atan(index);
}

int32_t fast_inv_sqrt(int32_t value)
{
    int32_t x2, y;
    const int32_t threehalfs = 96;
    x2 = value >> 1;

    y = 12288 - (value >> 1);

    int32_t ySquared = fixed_pow(y);
    int32_t product = fixed_mul(x2, ySquared);
    int32_t correction = threehalfs - product;
    y = fixed_mul(y, correction);

    return y;
}

int32_t fast_sqrt(int64_t value)
{
    if (value <= 0)
        return 0;
    int64_t x = value;
    for (uint8_t i = 0; i < 5; i++)
    {
        x = (x + fixed_div(value, x)) >> 1;
    }
    return x;
}

int32_t radian_to_index(int32_t radian)
{
    while (radian < 0)
        radian += PI2;
    while (radian >= PI2)
        radian -= PI2;
    return (radian * RADIAN_INDEX_FACTOR) >> SHIFT_FACTOR;
}