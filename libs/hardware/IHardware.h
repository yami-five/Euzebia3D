#ifndef IHARDWARE_h
#define IHARDWARE_h

#include <stdint.h>
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"

// typedef struct
// {
//     uint32_t sample_freq;
//     uint16_t channel_count;
//     uint8_t audio_data;
//     uint8_t audio_clock;
//     PIO pio;
//     uint8_t sm;
// } audio_format_t;

typedef struct
{
    void (*init_hardware)(void);
    void (*init_audio_i2s)(void);
    void (*write)(uint32_t pin, uint8_t value);
    void (*spi_write_byte)(uint8_t value);
    uint8_t (*spi_write_read_byte)(uint8_t value);
    void (*delay_ms)(uint32_t ms);
    void (*set_pwm)(uint8_t value);
    spi_inst_t *(*get_spi_port)(void);
    void (*set_spi_port)(uint8_t spiNum);
    audio_buffer_pool_t *(*get_audio_buffer_pool)(void);
    spin_lock_t *(*get_spinlock)(void);
    void (*set_lcd_cs_pin_high)(void);
    void (*set_lcd_cs_pin_low)(void);
} IHardware;

#endif