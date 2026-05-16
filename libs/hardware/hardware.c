#include "hardware.h"
#include <assert.h>
#include <stddef.h>

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "../storage/pins.h"
#include "pico/audio_i2s.h"
#include "hardware/dma.h"

volatile bool te_signal_detected = false;
static uint32_t slice_num;
static spin_lock_t *spi_spinlock;

#define SAMPLES_PER_BUFFER 256

bool get_te_signal_detected()
{
    return te_signal_detected;
}

void set_te_signal_detected(bool value)
{
    te_signal_detected = value;
}
#endif

static void init_hardware(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return;
#else
    stdio_init_all();

    // SPI config
    spi_init(spi0, 100000 * 1000);
    spi_init(spi1, 100000 * 1000);
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MISO_PIN, GPIO_FUNC_SPI);

    // PWM config
    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(LCD_BL_PIN);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);

    // I2C config
    i2c_init(i2c1, 300 * 1000);
    gpio_set_function(LCD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_SDA_PIN);
    gpio_pull_up(LCD_SCL_PIN);

    // GPIO config
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LCD_RST_PIN);
    gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
    gpio_init(LCD_DC_PIN);
    gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    gpio_init(LCD_BL_PIN);
    gpio_set_dir(LCD_BL_PIN, GPIO_OUT);
    gpio_init(LCD_CS_PIN);
    gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
    gpio_init(TP_CS_PIN);
    gpio_set_dir(TP_CS_PIN, GPIO_OUT);
    gpio_init(TP_IRQ_PIN);
    gpio_set_dir(TP_IRQ_PIN, GPIO_IN);
    gpio_init(SD_CS_PIN);
    gpio_set_dir(SD_CS_PIN, GPIO_OUT);
    gpio_set_pulls(TP_IRQ_PIN, true, false);

    gpio_put(TP_CS_PIN, 1);
    gpio_put(LCD_CS_PIN, 1);
    gpio_put(LCD_BL_PIN, 1);
    gpio_put(SD_CS_PIN, 1);

    spi_spinlock = spin_lock_init(spin_lock_claim_unused(true));
#endif
}

static void init_audio_i2s(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return;
#else
    static struct audio_format format = {
        .sample_freq = 96000,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2,
    };

    static struct audio_buffer_format producer_format = {
        .format = &format,
        .sample_stride = 16,
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(
        &producer_format,
        16,
        SAMPLES_PER_BUFFER
    );
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
        .data_pin = PICO_AUDIO_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_CLOCK_PIN,
        .dma_channel = 10,
        .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&format, &config);
    if (!output_format)
        panic("PicoAudio: Unable to open audio device.\n");

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);

    audio_i2s = producer_pool;
#endif
}

static void write(uint32_t pin, uint8_t value)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)pin;
    (void)value;
#else
    gpio_put(pin, value);
#endif
}

static void spi_write_byte(uint8_t value)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)value;
#else
    spi_write_blocking(SPI_PORT, &value, 1);
#endif
}

static uint8_t spi_write_read_byte(uint8_t value)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)value;
    return 0;
#else
    uint8_t rx_data;
    spi_write_read_blocking(SPI_PORT, &value, &rx_data, 1);
    return rx_data;
#endif
}

static void delay_ms(uint32_t ms)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)ms;
#else
    sleep_ms(ms);
#endif
}

static void set_pwm(uint8_t value)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)value;
#else
    if (value <= 100)
        pwm_set_chan_level(slice_num, PWM_CHAN_B, value);
#endif
}

static spi_inst_t *get_spi_port(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return NULL;
#else
    return SPI_PORT;
#endif
}

static void set_spi_port(uint8_t spi_num)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)spi_num;
#else
    if (spi_num == 0)
        SPI_PORT = spi0;
    else
        SPI_PORT = spi1;
#endif
}

static audio_buffer_pool_t *get_audio_buffer_pool(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return NULL;
#else
    return audio_i2s;
#endif
}

static spin_lock_t *get_spinlock(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return NULL;
#else
    return spi_spinlock;
#endif
}

static void set_lcd_cs_pin_high(void)
{
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
    gpio_put(LCD_CS_PIN, 1);
#endif
}

static void set_lcd_cs_pin_low(void)
{
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
    gpio_put(LCD_CS_PIN, 0);
#endif
}

static IHardware hardware = {
    .init_hardware = init_hardware,
    .init_audio_i2s = init_audio_i2s,
    .write = write,
    .spi_write_byte = spi_write_byte,
    .spi_write_read_byte = spi_write_read_byte,
    .delay_ms = delay_ms,
    .set_pwm = set_pwm,
    .get_spi_port = get_spi_port,
    .set_spi_port = set_spi_port,
    .get_audio_buffer_pool = get_audio_buffer_pool,
    .get_spinlock = get_spinlock,
    .set_lcd_cs_pin_high = set_lcd_cs_pin_high,
    .set_lcd_cs_pin_low = set_lcd_cs_pin_low,
};

const IHardware *get_hardware(void)
{
    return &hardware;
}
