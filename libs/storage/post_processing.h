#ifndef POSTPROCESSING_H
#define POSTPROCESSING_H

#include "stdint.h"

const uint32_t *get_effect_table(uint8_t effect_index);
const uint32_t get_effect_table_element(uint8_t effect_index, uint32_t e_index);

#endif