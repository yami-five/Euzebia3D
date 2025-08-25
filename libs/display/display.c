#include "IDisplay.h"
#include "display.h"
#include "../shared/pins.h"

static const uint16_t DISPLAY_WIDTH = 320;
static const uint16_t DISPLAY_HEIGHT = 240;
static const uint8_t HORIZONTAL=0;
static const uint8_t VERTICAL=1;
static volatile const IHardware *_hardware = NULL;

void set_hardware(IHardware* hardware)
{
    hardware=hardware;    
}

static void send_command(uint8_t reg)
{
    _hardware->write(LCD_DC_PIN, 0);
    _hardware->write(LCD_CS_PIN, 0);
    _hardware->spi_write_byte(reg);
    _hardware->write(LCD_CS_PIN, 1);
}

static void send_data_8bit(uint8_t data)
{
    _hardware->write(LCD_DC_PIN, 1);
    _hardware->write(LCD_CS_PIN, 0);
    _hardware->spi_write_byte(data);
    _hardware->write(LCD_CS_PIN, 1);
}

static void send_data_16bit(uint16_t data)
{
    _hardware->write(LCD_DC_PIN, 1);
    _hardware->write(LCD_CS_PIN, 0);
    _hardware->spi_write_byte((data >> 8) & 0xFF);
    _hardware->spi_write_byte(data & 0xFF);
    _hardware->write(LCD_CS_PIN, 1);
}

static void set_attributes(uint8_t scan_dir)
{
    uint8_t memory_access_reg = 0x00;
    if(scan_dir == HORIZONTAL) {
        memory_access_reg = 0X70;
    } else {
        memory_access_reg = 0X00;
    }

    // Set the read / write scan direction of the frame memory
    send_command(0x36); //MX, MY, RGB mode
    send_data_8bit(memory_access_reg);	//0x08 set RGB
}

void lcd_reset(void)
{
    _hardware->write(LCD_RST_PIN, 1);
    _hardware->delay_ms(100);
    _hardware->write(LCD_RST_PIN, 0);
    _hardware->delay_ms(100);
    _hardware->write(LCD_RST_PIN, 1);
    _hardware->delay_ms(100);
}

void set_windows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    //set the X coordinates
    send_command(0x2A);
    send_data_8bit(Xstart >>8);
    send_data_8bit(Xstart & 0xff);
	send_data_8bit((Xend - 1) >> 8);
    send_data_8bit((Xend-1) & 0xFF);

    //set the Y coordinates
    send_command(0x2B);
    send_data_8bit(Ystart >>8);
	send_data_8bit(Ystart & 0xff);
	send_data_8bit((Yend - 1) >> 8);
    send_data_8bit((Yend - 1) & 0xff);

    send_command(0X2C);
}

static void init_display(volatile const IHardware* hardware)
{
	_hardware=hardware;
    _hardware->set_pwm(100);
    lcd_reset();
    set_attributes(0);
    send_command(0x36);
	send_data_8bit(0x00); 

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
    
    set_windows(0, 0, DISPLAY_HEIGHT, DISPLAY_WIDTH);
}

static IDisplay display = {
    .init_display = init_display
};

const IDisplay *get_display(void)
{
    return &display;
}
