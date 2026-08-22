#include "hardware.h"
#include <assert.h>
#include <stddef.h>

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "../storage/pins.h"
#include "hardware/dma.h"
#include "pico/audio_i2s.h"

static uint32_t slice_num;
static uint32_t lcd_spi_baudrate_hz;
static spin_lock_t *spi_spinlock;

#define SAMPLES_PER_BUFFER 256
#define LCD_SPI_REQUESTED_BAUDRATE_HZ (100000u * 1000u)
#define SD_SPI_INITIAL_BAUDRATE_HZ (400u * 1000u)
#endif

static void init_hardware(void) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  return;
#else
  uint32_t sys_clock_hz = clock_get_hz(clk_sys);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  sys_clock_hz, sys_clock_hz);

  stdio_init_all();

  // SPI config
  uint32_t actual_sd_spi_baud = spi_init(spi0, SD_SPI_INITIAL_BAUDRATE_HZ);
  lcd_spi_baudrate_hz = spi_init(spi1, LCD_SPI_REQUESTED_BAUDRATE_HZ);
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  printf("LCD spi1 requested: %lu Hz, actual: %lu Hz; SD spi0 requested: %lu "
         "Hz, actual: %lu Hz\n",
         (unsigned long)LCD_SPI_REQUESTED_BAUDRATE_HZ,
         (unsigned long)lcd_spi_baudrate_hz,
         (unsigned long)SD_SPI_INITIAL_BAUDRATE_HZ,
         (unsigned long)actual_sd_spi_baud);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_MISO_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_MISO_PIN, GPIO_FUNC_SPI);
  gpio_pull_up(SD_MISO_PIN);

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
  gpio_init(ONBOARD_SD_CS_PIN);
  gpio_set_dir(ONBOARD_SD_CS_PIN, GPIO_OUT);
  gpio_set_pulls(TP_IRQ_PIN, true, false);

  gpio_put(TP_CS_PIN, 1);
  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_BL_PIN, 1);
  gpio_put(SD_CS_PIN, 1);
  gpio_put(ONBOARD_SD_CS_PIN, 1);

  spi_spinlock = spin_lock_init(spin_lock_claim_unused(true));
#endif
}

static void init_audio_i2s(uint32_t sample_freq, uint16_t channel_count) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)sample_freq;
  (void)channel_count;
  return;
#else
  if (audio_i2s != NULL)
    return;

  static struct audio_format format;
  static struct audio_buffer_format producer_format;

  format.sample_freq = sample_freq;
  format.format = AUDIO_BUFFER_FORMAT_PCM_S16;
  format.channel_count = channel_count;

  producer_format.format = &format;
  producer_format.sample_stride =
      (uint16_t)(sizeof(int16_t) * channel_count);

  struct audio_buffer_pool *producer_pool =
      audio_new_producer_pool(&producer_format, 16, SAMPLES_PER_BUFFER);
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

static void write(uint32_t pin, uint8_t value) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)pin;
  (void)value;
#else
  gpio_put(pin, value);
#endif
}

static void lcd_spi_write_byte(uint8_t value) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)value;
#else
  spi_write_blocking(spi1, &value, 1);
#endif
}

static uint8_t sd_spi_write_read_byte(uint8_t value) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)value;
  return 0;
#else
  uint8_t rx_data;
  spi_write_read_blocking(spi0, &value, &rx_data, 1);
  return rx_data;
#endif
}

static void set_sd_spi_baudrate_hz(uint32_t baudrate_hz) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)baudrate_hz;
#else
  spi_set_baudrate(spi0, baudrate_hz);
#endif
}

static void delay_ms(uint32_t ms) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)ms;
#else
  sleep_ms(ms);
#endif
}

static void set_pwm(uint8_t value) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  (void)value;
#else
  if (value <= 100)
    pwm_set_chan_level(slice_num, PWM_CHAN_B, value);
#endif
}

static spi_inst_t *get_lcd_spi_port(void) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  return NULL;
#else
  return spi1;
#endif
}

static audio_buffer_pool_t *get_audio_buffer_pool(void) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  return NULL;
#else
  return audio_i2s;
#endif
}

static spin_lock_t *get_spinlock(void) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  return NULL;
#else
  return spi_spinlock;
#endif
}

static uint32_t get_lcd_spi_baudrate_hz(void) {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
  return 0u;
#else
  return lcd_spi_baudrate_hz;
#endif
}

static void set_lcd_cs_pin_high(void) {
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
  gpio_put(LCD_CS_PIN, 1);
#endif
}

static void set_lcd_cs_pin_low(void) {
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
  gpio_put(LCD_CS_PIN, 0);
#endif
}

static e3d_IHardware hardware = {
    .init_hardware = init_hardware,
    .init_audio_i2s = init_audio_i2s,
    .write = write,
    .lcd_spi_write_byte = lcd_spi_write_byte,
    .sd_spi_write_read_byte = sd_spi_write_read_byte,
    .set_sd_spi_baudrate_hz = set_sd_spi_baudrate_hz,
    .delay_ms = delay_ms,
    .set_pwm = set_pwm,
    .get_lcd_spi_port = get_lcd_spi_port,
    .get_audio_buffer_pool = get_audio_buffer_pool,
    .get_spinlock = get_spinlock,
    .get_lcd_spi_baudrate_hz = get_lcd_spi_baudrate_hz,
    .set_lcd_cs_pin_high = set_lcd_cs_pin_high,
    .set_lcd_cs_pin_low = set_lcd_cs_pin_low,
};

const e3d_IHardware *get_hardware(void) { return &hardware; }
