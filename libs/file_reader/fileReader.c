#include "IFileReader.h"
#include "fileReader.h"

static const IHardware *_hardware = NULL;

#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "fatfs/ff.h"
#include "stdio.h"
#include "../storage/pins.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include "string.h"
#include "hardware/sync/spin_lock.h"
#include "pico/stdlib.h"

static FRESULT f_res;
static FATFS microSDFatFs;

static void sd_init(void)
{
    _hardware->write(SD_CS_PIN, 1);
    _hardware->write(LCD_CS_PIN, 1);
    _hardware->write(TP_CS_PIN, 1);

    // Check the mounted device.
    f_res = f_mount(&microSDFatFs, (TCHAR const *)"/", 1);
    if (f_res != FR_OK)
        printf("SD card mount file system failed ,error code :(%d)\r\n", f_res);
    else
        printf("SD card mount file system success!! \r\n");
}
#endif

static void play_wave_file(char *file_name)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)file_name;
#else
    FIL file;
    uint8_t header[44];
    UINT br;

    f_res = f_open(&file, file_name, FA_READ);
    if (f_res != FR_OK)
    {
        printf("Loading file failed :(%d)\r\n", f_res);
        return;
    }

    f_lseek(&file, 0);
    f_read(&file, header, sizeof(header), &br);

    uint16_t num_channels = header[22] | (header[23] << 8);
    uint16_t bits_per_sample = header[34] | (header[35] << 8);
    uint32_t data_size = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
    uint32_t bytes_per_sample = bits_per_sample / 8;
    uint32_t sample_count = data_size / (num_channels * bytes_per_sample);
    (void)sample_count;

    uint16_t samples_num = 1;
    uint16_t buffer_size = 16;
    int16_t buffer_audio[16];

    while (1)
    {
        f_read(&file, buffer_audio, sizeof(buffer_audio), &br);
        if (br == 0)
            break;

        for (uint16_t i = 0; i < samples_num; i++)
        {
            struct audio_buffer_pool *audio_buffer_pool = _hardware->get_audio_buffer_pool();
            struct audio_buffer *audio_buf = take_audio_buffer(audio_buffer_pool, true);
            memcpy(audio_buf->buffer->bytes, (buffer_audio + buffer_size * i), 32);
            audio_buf->sample_count = 16;
            give_audio_buffer(audio_buffer_pool, audio_buf);
            if (br == 0)
                break;
        }
    }

    f_close(&file);
#endif
}

static void init_fileReader(const IHardware *hardware)
{
    _hardware = hardware;
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
    sd_init();
#endif
}

static IFileReader fileReader = {
    .init_fileReader = init_fileReader,
    .play_wave_file = play_wave_file,
};

const IFileReader *get_fileReader(void)
{
    return &fileReader;
}
