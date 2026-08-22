#ifndef HARDWARE_h
#define HARDWARE_h

#include "IHardware.h"

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "../storage/pins.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/audio_i2s.h"
#include "pico/stdlib.h"
#include "stdio.h"

static struct audio_buffer_pool *audio_i2s;
#endif

const e3d_IHardware *get_hardware(void);

#endif
