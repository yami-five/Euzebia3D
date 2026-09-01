#include "painter_platform.h"
#include "painter.h"

#include "../../storage/pins.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stddef.h>

static const e3d_IHardware *painter_hardware = NULL;
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
      &config, spi_get_dreq(painter_hardware->get_lcd_spi_port(), true));
  dma_channel_configure(
      dma_channel, &config,
      &spi_get_hw(painter_hardware->get_lcd_spi_port())->dr, NULL,
      BUFFER_SIZE_HALF, false);
  dma_channel_set_irq1_enabled(dma_channel, true);
  irq_set_exclusive_handler(DMA_IRQ_1, dma_buffer_irq_handler);
  irq_set_enabled(DMA_IRQ_1, false);
}

void painter_platform_init(const e3d_IDisplay *display,
                           const e3d_IHardware *hardware) {
  (void)display;
  painter_hardware = hardware;
  init_dma();
  painter_hardware->write(LCD_CS_PIN, 0);
}

bool painter_platform_draw_buffer(const uint16_t *buffer,
                                  volatile uint32_t *debug_stage,
                                  volatile uint32_t *debug_line) {
  (void)debug_line;

  set_debug_stage(debug_stage, 110);
  spi_inst_t *spi_port = painter_hardware->get_lcd_spi_port();
  (void)painter_hardware->get_spinlock();

  set_debug_stage(debug_stage, 120);
  spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  painter_hardware->write(LCD_DC_PIN, 0);
  painter_hardware->lcd_spi_write_byte(0x2C);
  painter_hardware->write(LCD_DC_PIN, 1);
  spi_set_format(spi_port, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  set_debug_stage(debug_stage, 130);
  dma_channel_set_trans_count(dma_channel, BUFFER_SIZE_HALF, false);
  dma_channel_set_read_addr(dma_channel, buffer, true);
  set_debug_stage(debug_stage, 140);
  dma_channel_wait_for_finish_blocking(dma_channel);

  set_debug_stage(debug_stage, 150);
  while (spi_is_busy(spi_port)) {
  }

  set_debug_stage(debug_stage, 160);
  spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  return true;
}

void painter_platform_draw_image(uint16_t *buffer, const e3d_Image *image) {
  int dma_channel_flash = dma_claim_unused_channel(true);
  dma_channel_config config = dma_channel_get_default_config(dma_channel_flash);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, true);
  dma_channel_configure(dma_channel_flash, &config, buffer, image->image,
                        BUFFER_SIZE_HALF, false);
  dma_channel_start(dma_channel_flash);
  dma_channel_wait_for_finish_blocking(dma_channel_flash);
  dma_channel_unclaim(dma_channel_flash);
}
