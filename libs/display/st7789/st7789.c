#include "display.h"

#include "../../storage/pins.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include <stddef.h>
#include "../../storage/pins.h"

#define WIDTH_DOUBLED 640
#define HEIGHT_DOUBLED 480
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define WIDTH_HALF 160
#define HEIGHT_HALF 120
#define BUFFER_SIZE 153600
#define BUFFER_SIZE_HALF 76800
#define HORIZONTAL 0
static const e3d_IHardware *_hardware;

static uint dma_channel;

static void set_debug_stage(volatile uint32_t *debug_stage, uint32_t stage) {
  if (debug_stage != NULL)
    *debug_stage = stage;
}

static void dma_buffer_irq_handler(void) { dma_hw->ints1 = 1u << dma_channel; }

static void init_dma(void) {
  dma_channel = dma_claim_unused_channel(true);
  dma_channel_config config = dma_channel_get_default_config(dma_channel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, false);
  channel_config_set_dreq(
      &config, spi_get_dreq(_hardware->get_lcd_spi_port(), true));
  dma_channel_configure(dma_channel, &config,
                        &spi_get_hw(_hardware->get_lcd_spi_port())->dr,
                        NULL, BUFFER_SIZE_HALF, false);
  dma_channel_set_irq1_enabled(dma_channel, true);
  irq_set_exclusive_handler(DMA_IRQ_1, dma_buffer_irq_handler);
  irq_set_enabled(DMA_IRQ_1, false);
}

static void send_command(uint8_t reg) {
  _hardware->write(LCD_DC_PIN, 0);
  _hardware->write(LCD_CS_PIN, 0);
  _hardware->lcd_spi_write_byte(reg);
  _hardware->write(LCD_CS_PIN, 1);
}

static void send_data_8bit(uint8_t data) {
  _hardware->write(LCD_DC_PIN, 1);
  _hardware->write(LCD_CS_PIN, 0);
  _hardware->lcd_spi_write_byte(data);
  _hardware->write(LCD_CS_PIN, 1);
}

static void set_attributes(uint8_t scan_dir) {
  uint8_t memory_access_reg = 0x00;
  if (scan_dir == HORIZONTAL) {
    // Landscape without mirror: axis swap only.
    memory_access_reg = 0x20;
  }

  // Set the read / write scan direction of the frame memory
  send_command(0x36); // MX, MY, RGB mode
  send_data_8bit(memory_access_reg);
}

static void lcd_reset(void) {
  _hardware->write(LCD_RST_PIN, 1);
  _hardware->delay_ms(100);
  _hardware->write(LCD_RST_PIN, 0);
  _hardware->delay_ms(100);
  _hardware->write(LCD_RST_PIN, 1);
  _hardware->delay_ms(100);
}

static void set_windows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
                        uint16_t Yend) {
  // set the X coordinates
  send_command(0x2A);
  send_data_8bit(Xstart >> 8);
  send_data_8bit(Xstart & 0xff);
  send_data_8bit((Xend - 1) >> 8);
  send_data_8bit((Xend - 1) & 0xFF);

  // set the Y coordinates
  send_command(0x2B);
  send_data_8bit(Ystart >> 8);
  send_data_8bit(Ystart & 0xff);
  send_data_8bit((Yend - 1) >> 8);
  send_data_8bit((Yend - 1) & 0xff);

  send_command(0X2C);
}

void display_init(const e3d_IHardware *hardware) {
  _hardware = hardware;
  _hardware->set_pwm(100);
  lcd_reset();
  set_attributes(HORIZONTAL);

  send_command(0x3A);
  send_data_8bit(0x05);

  send_command(0x21);

  send_command(0x2A);
  send_data_8bit(0x00);
  send_data_8bit(0x00);
  send_data_8bit(0x01);
  send_data_8bit(0x3F);

  send_command(0x2B);
  send_data_8bit(0x00);
  send_data_8bit(0x00);
  send_data_8bit(0x00);
  send_data_8bit(0xEF);

  send_command(0xB2);
  send_data_8bit(0x0C);
  send_data_8bit(0x0C);
  send_data_8bit(0x00);
  send_data_8bit(0x33);
  send_data_8bit(0x33);

  send_command(0xB7);
  send_data_8bit(0x35);

  send_command(0xBB);
  send_data_8bit(0x1F);

  send_command(0xC0);
  send_data_8bit(0x2C);

  send_command(0xC2);
  send_data_8bit(0x01);

  send_command(0xC3);
  send_data_8bit(0x12);

  send_command(0xC4);
  send_data_8bit(0x20);

  send_command(0xC6);
  send_data_8bit(0x0F);

  send_command(0xD0);
  send_data_8bit(0xA4);
  send_data_8bit(0xA1);

  send_command(0xE0);
  send_data_8bit(0xD0);
  send_data_8bit(0x08);
  send_data_8bit(0x11);
  send_data_8bit(0x08);
  send_data_8bit(0x0C);
  send_data_8bit(0x15);
  send_data_8bit(0x39);
  send_data_8bit(0x33);
  send_data_8bit(0x50);
  send_data_8bit(0x36);
  send_data_8bit(0x13);
  send_data_8bit(0x14);
  send_data_8bit(0x29);
  send_data_8bit(0x2D);

  send_command(0xE1);
  send_data_8bit(0xD0);
  send_data_8bit(0x08);
  send_data_8bit(0x10);
  send_data_8bit(0x08);
  send_data_8bit(0x06);
  send_data_8bit(0x06);
  send_data_8bit(0x39);
  send_data_8bit(0x44);
  send_data_8bit(0x51);
  send_data_8bit(0x0B);
  send_data_8bit(0x16);
  send_data_8bit(0x14);
  send_data_8bit(0x2F);
  send_data_8bit(0x31);
  send_command(0x21);

  send_command(0x11);
  send_command(0x29);
  send_command(0x35);

  set_windows(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  init_dma();
  _hardware->write(LCD_CS_PIN, 0);
}

bool display_present_framebuffer(const uint16_t *framebuffer,
                                 volatile uint32_t *debug_stage,
                                 volatile uint32_t *debug_line) {
  (void)debug_line;

  set_debug_stage(debug_stage, 110);
  spi_inst_t *spi_port = _hardware->get_lcd_spi_port();
  (void)_hardware->get_spinlock();

  set_debug_stage(debug_stage, 120);
  spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  _hardware->write(LCD_DC_PIN, 0);
  _hardware->lcd_spi_write_byte(0x2C);
  _hardware->write(LCD_DC_PIN, 1);
  spi_set_format(spi_port, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  set_debug_stage(debug_stage, 130);
  dma_channel_set_trans_count(dma_channel, BUFFER_SIZE_HALF, false);
  dma_channel_set_read_addr(dma_channel, framebuffer, true);
  set_debug_stage(debug_stage, 140);
  dma_channel_wait_for_finish_blocking(dma_channel);

  set_debug_stage(debug_stage, 150);
  while (spi_is_busy(spi_port)) {
  }

  set_debug_stage(debug_stage, 160);
  spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  return true;
}