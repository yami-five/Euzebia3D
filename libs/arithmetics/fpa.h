#include <math.h>
#include <stdint.h>

#define SHIFT_FACTOR 12
#define SCALE_FACTOR (1 << SHIFT_FACTOR)
#define TABLE_SIZE 36000
#define PI 12867//3.14159265358979323846f * 1024
#define PI2 25734 //PI * 2
// #define PI_FIXED PI *SCALE_FACTOR
// #define PI2_FIXED PI2 *SCALE_FACTOR
#define RESOLUTION (PI2 / TABLE_SIZE)
#define RESOLUTION_FIXED RESOLUTION *SCALE_FACTOR
#define RADIAN_INDEX_FACTOR 5729 //table_size/pi2

int32_t float_to_fixed(float value);
int32_t fixed_to_float(int32_t value);
int32_t fixed_add(int32_t a, int32_t b);
int32_t fixed_add3(int32_t a, int32_t b, int32_t c);
int32_t fixed_sub(int32_t a, int32_t b);
int64_t fixed_mul(int32_t a, int32_t b);
int32_t fixed_div(int32_t a, int32_t b);
int64_t fixed_pow(int32_t a);
// void init_sin_cos();
int16_t fast_sin(int32_t value);
int16_t fast_cos(int32_t value);
int16_t fast_atan2(int16_t y, int16_t x);
int32_t fast_inv_sqrt(int32_t number);
int32_t fast_sqrt(int64_t number);
int32_t radian_to_index(int32_t radian);
int32_t inverse(int32_t number);