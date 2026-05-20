#include <stdint.h>

#define EUZEBIA3D_SIN_TABLE_LENGTH 36000u
#define EUZEBIA3D_COS_TABLE_LENGTH 36000u
#define EUZEBIA3D_ATAN_TABLE_LENGTH 20164u

const int16_t get_sin(uint16_t index);
const int16_t get_cos(uint16_t index);
const int16_t get_atan(uint16_t index);

void set_lookup_table_sources(const int16_t *sin_source, const int16_t *cos_source, const int16_t *atan_source);
void reset_lookup_table_sources(void);

const int16_t *get_sin_source(void);
const int16_t *get_cos_source(void);
const int16_t *get_atan_source(void);

