#include "IFileReader.h"
#include "fileReader.h"
#include "fatfs/ff.h"
#include "stdio.h"
#include "../shared/pins.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include "string.h"
#include "hardware/sync/spin_lock.h"
#include "pico/stdlib.h"

static const IHardware *_hardware = NULL;
static FRESULT f_res;
static FATFS microSDFatFs;
static spin_lock_t *sd_spinlock;

uint8_t readline(uint8_t *line, uint8_t length, FIL *file)
{
	uint br;
	uint8_t lineBuffer[1];
	f_read(file, lineBuffer, 1, &br);
	if (f_eof(file))
	{
		return 1;
	}
	line[0] = lineBuffer[0];
	uint8_t counter = 1;
	while ((lineBuffer[0] != '\n') && (counter < length - 1))
	{
		f_read(file, lineBuffer, 1, &br);
		line[counter] = lineBuffer[0];
		counter++;
	}
	return 0;
}

void clearline(uint8_t *line, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++)
	{
		line[i] = 0;
	}
}

void sd_init()
{
	_hardware->write(SD_CS_PIN, 1);
	_hardware->write(LCD_CS_PIN, 1);
	_hardware->write(TP_CS_PIN, 1);

	// Check the mounted device
	f_res = f_mount(&microSDFatFs, (TCHAR const *)"/", 1);
	if (f_res != FR_OK)
	{
		printf("SD card mount file system failed ,error code :(%d)\r\n", f_res);
	}
	else
	{
		printf("SD card mount file system success!! \r\n");
	}
}

void sd_close()
{
	f_unmount((TCHAR const *)"/");
}

void play_wave_file(char *file_name)
{
	FIL file;
	uint8_t header[44];
	f_res = f_open(&file, file_name, FA_READ);
	uint br;
	f_lseek(&file, 0);
	f_read(&file, header, 44, &br);
	if (f_res != FR_OK)
	{
		printf("Loading file failed :(%d)\r\n", f_res);
		return;
	}
	uint16_t num_channels = header[22] | (header[23] << 8);
    uint16_t bits_per_sample = header[34] | (header[35] << 8);
    uint32_t data_size = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);

    uint32_t bytes_per_sample = bits_per_sample / 8;
    uint32_t sample_count = data_size / (num_channels * bytes_per_sample);
	// const int buffer_size = 16;
    // int16_t buffer[buffer_size];
	uint16_t samples_num = 1;
	uint16_t buffer_size = 16;
	int16_t buffer_audio[buffer_size*samples_num];
	spin_lock_t *spi_spinlock = _hardware->get_spinlock();
	int16_t temp_buffer[buffer_size];
	while (1)
	{
		// uint32_t flags = spin_lock_blocking(spi_spinlock);
		f_read(&file, buffer_audio, sizeof(buffer_audio), &br);
		// spin_unlock(spi_spinlock, flags);
		if (br == 0)
			break;
		for (uint16_t i = 0; i < samples_num; i++)
		{
			struct audio_buffer_pool *audio_buffer_pool = _hardware->get_audio_buffer_pool();
			struct audio_buffer *audio_buf = take_audio_buffer(audio_buffer_pool, true);
			memcpy(audio_buf->buffer->bytes, (buffer_audio + buffer_size*i), 32);
			audio_buf->sample_count = 16;
			give_audio_buffer(audio_buffer_pool, audio_buf);
			// sleep_us(100);
			if (br == 0)
				break;
		}
	}

	f_close(&file);
}

void init_fileReader(const IHardware *hardware)
{
	_hardware = hardware;
	sd_init();
}

static IFileReader fileReader = {
	.init_fileReader = init_fileReader,
	.play_wave_file = play_wave_file};

const IFileReader *get_fileReader(void)
{
	return &fileReader;
}