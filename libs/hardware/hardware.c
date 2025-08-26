#include "IHardware.h"
#include "hardware.h"
#include "../storage/pins.h"
#include "pico/audio_i2s.h"
#include "hardware/dma.h"

volatile bool te_signal_detected = false;
uint32_t slice_num;
static spin_lock_t *spi_spinlock;
static uint8_t sio_dma_channel_high;
static uint8_t sio_dma_channel_low;
static const uint32_t cs_bit[] = {1 << LCD_CS_PIN};

#define SAMPLES_PER_BUFFER 256
#define LCD_CS_MASK (1u << LCD_CS_PIN)
#define SD_CS_MASK (1u << SD_CS_PIN)

void init_audio_i2s()
{
    static struct audio_format format = {
        .sample_freq = 96000,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2};

    static struct audio_buffer_format producer_format = {
        .format = &format,
        .sample_stride = 16};

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 16,
                                                                      SAMPLES_PER_BUFFER);
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
        .data_pin = PICO_AUDIO_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_CLOCK_PIN,
        .dma_channel = 10,
        .pio_sm = 0};

    output_format = audio_i2s_setup(&format, &config);
    if (!output_format)
    {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);

    audio_i2s = producer_pool;
}

bool get_te_signal_detected()
{
    return te_signal_detected;
}
void set_te_signal_detected(bool value)
{
    te_signal_detected = value;
}

static void write(uint32_t pin, uint8_t value)
{
    gpio_put(pin, value);
}

static uint8_t read(uint32_t pin)
{
    gpio_get(pin);
}

static void spi_write_byte(uint8_t value)
{
    spi_write_blocking(SPI_PORT, &value, 1);
}

static uint8_t spi_write_read_byte(uint8_t value)
{
    uint8_t rxDat;
    spi_write_read_blocking(SPI_PORT, &value, &rxDat, 1);
    return rxDat;
}

void gpio_mode(uint16_t pin, uint16_t mode)
{
    gpio_init(pin);
    if (mode == 0 || mode == GPIO_IN)
    {
        gpio_set_dir(pin, GPIO_IN);
    }
    else
    {
        gpio_set_dir(pin, GPIO_OUT);
    }
}

static void delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}

static void set_pwm(uint8_t value)
{
    if (value < 0 || value > 100)
    {
        printf("DEV_SET_PWM Error \r\n");
    }
    else
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, value);
    }
}

static void init_hardware(void)
{

    stdio_init_all();

    // SPI Config
    spi_init(spi0, 100000 * 1000);
    spi_init(spi1, 100000 * 1000);
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MISO_PIN, GPIO_FUNC_SPI);

    // PWM Config
    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(LCD_BL_PIN);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);

    // I2C Config
    i2c_init(i2c1, 300 * 1000);
    gpio_set_function(LCD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_SDA_PIN);
    gpio_pull_up(LCD_SCL_PIN);

    // GPIO Config
    gpio_mode(LED_PIN, GPIO_OUT);

    gpio_mode(LCD_RST_PIN, GPIO_OUT);
    gpio_mode(LCD_DC_PIN, GPIO_OUT);
    gpio_mode(LCD_BL_PIN, GPIO_OUT);
    gpio_mode(LCD_CS_PIN, GPIO_OUT);
    gpio_mode(TP_CS_PIN, GPIO_OUT);
    gpio_mode(TP_IRQ_PIN, GPIO_IN);
    gpio_mode(SD_CS_PIN, GPIO_OUT);
    gpio_set_pulls(TP_IRQ_PIN, true, false);

    write(TP_CS_PIN, GPIO_OUT);
    write(LCD_CS_PIN, GPIO_OUT);
    write(LCD_BL_PIN, GPIO_OUT);
    write(SD_CS_PIN, GPIO_OUT);

    spi_spinlock = spin_lock_init(spin_lock_claim_unused(true));
}

spi_inst_t *get_spi_port()
{
    return SPI_PORT;
}

void set_spi_port(uint8_t spiNum)
{
    if (spiNum == 0)
        SPI_PORT = spi0;
    else
        SPI_PORT = spi1;
}

audio_buffer_pool_t *get_audio_buffer_pool()
{
    return audio_i2s;
}

spin_lock_t *get_spinlock()
{
    return spi_spinlock;
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
    .get_spinlock = get_spinlock};

const IHardware *get_hardware(void)
{
    return &hardware;
}
