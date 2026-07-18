#include "fpa.h"
#include "lookup_tables.h"
#include <limits.h>

int32_t float_to_fixed(float value) { return (int32_t)(value * SCALE_FACTOR); }

int32_t fixed_to_float(int32_t value) { return (float)(value >> SHIFT_FACTOR); }

int32_t fixed_add(int32_t a, int32_t b) { return a + b; }

int32_t fixed_add3(int32_t a, int32_t b, int32_t c) { return a + b + c; }

int32_t fixed_sub(int32_t a, int32_t b) { return a - b; }

int64_t fixed_mul(int32_t a, int32_t b) {
  int64_t result = (int64_t)a * b;
  return (int64_t)(result >> SHIFT_FACTOR);
}

int32_t fixed_div(int32_t a, int32_t b) {
  if (b == 0)
    return 0;

  int64_t result = ((int64_t)a << SHIFT_FACTOR) / b;
  if (result > INT32_MAX)
    return INT32_MAX;
  if (result < INT32_MIN)
    return INT32_MIN;
  return (int32_t)result;
}

int64_t fixed_pow(int32_t a) { return fixed_mul(a, a); }

int16_t fast_sin(int32_t value) {
  int32_t index = value % TABLE_SIZE;
  if (index < 0)
    index += TABLE_SIZE;
  return get_sin(index);
}

int16_t fast_cos(int32_t value) {
  int32_t index = value % TABLE_SIZE;
  if (index < 0)
    index += TABLE_SIZE;
  return get_cos(index);
}

int16_t fast_atan2(int16_t y, int16_t x) {
  int32_t index = (y + 70) * 141 + x + 70;
  return get_atan(index);
}

int32_t fast_inv_sqrt(int32_t value) {
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

static uint64_t isqrt_u64(uint64_t value) {
  uint64_t result = 0;
  uint64_t bit = 1ull << 62;

  while (bit > value)
    bit >>= 2;

  while (bit != 0) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }

  return result;
}

int32_t fast_sqrt(int64_t value) {
  if (value <= 0)
    return 0;

  uint64_t result = isqrt_u64((uint64_t)value << SHIFT_FACTOR);
  if (result > INT32_MAX)
    return INT32_MAX;
  return (int32_t)result;
}

int32_t radian_to_index(int32_t radian) {
  while (radian < 0)
    radian += PI2;
  while (radian >= PI2)
    radian -= PI2;
  return (radian * RADIAN_INDEX_FACTOR) >> SHIFT_FACTOR;
}

int32_t inverse(int32_t number) { return fixed_div(SCALE_FACTOR, number); }
