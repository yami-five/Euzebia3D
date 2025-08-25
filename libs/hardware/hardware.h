#ifndef HARDWARE_h
#define HARDWARE_h

#include "IHardware.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "../shared/pins.h"
#include "pico/audio_i2s.h"

static spi_inst_t *SPI_PORT = spi1;
static spi_inst_t *I2C_PORT = spi1;
static struct audio_buffer_pool *audio_i2s;
const IHardware *get_hardware(void);

#endif