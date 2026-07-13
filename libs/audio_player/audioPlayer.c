#include "IAudioPlayer.h"
#include "audioPlayer.h"

#include <stddef.h>

static const IHardware *_hardware = NULL;

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>

static ma_engine audio_engine;
static ma_sound current_sound;
static int audio_engine_initialized = 0;
static int current_sound_initialized = 0;
static int audio_player_shutdown_registered = 0;

static void stop_current_sound(void)
{
    if (!current_sound_initialized)
    {
        return;
    }

    (void)ma_sound_stop(&current_sound);
    ma_sound_uninit(&current_sound);
    current_sound_initialized = 0;
}

static void audio_player_shutdown(void)
{
    stop_current_sound();

    if (!audio_engine_initialized)
    {
        return;
    }

    ma_engine_uninit(&audio_engine);
    audio_engine_initialized = 0;
}

static void audio_player_init(void)
{
    if (audio_engine_initialized)
    {
        return;
    }

    ma_result result = ma_engine_init(NULL, &audio_engine);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "miniaudio: failed to initialize engine (%d)\n", result);
        return;
    }

    audio_engine_initialized = 1;

    if (!audio_player_shutdown_registered && atexit(audio_player_shutdown) == 0)
    {
        audio_player_shutdown_registered = 1;
    }
}

static void play_wave_file(char *file_name)
{
    if (file_name == NULL || file_name[0] == '\0')
    {
        return;
    }

    if (!audio_engine_initialized)
    {
        audio_player_init();
    }

    if (!audio_engine_initialized)
    {
        return;
    }

    stop_current_sound();

    ma_result result = ma_sound_init_from_file(&audio_engine, file_name, 0, NULL, NULL, &current_sound);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "miniaudio: failed to load '%s' (%d)\n", file_name, result);
        return;
    }

    current_sound_initialized = 1;

    result = ma_sound_start(&current_sound);
    if (result != MA_SUCCESS)
    {
        fprintf(stderr, "miniaudio: failed to play '%s' (%d)\n", file_name, result);
        stop_current_sound();
    }
}
#elif defined(EUZEBIA3D_PLATFORM_PICO)
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

static void audio_player_init(void)
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

static void play_wave_file(char *file_name)
{
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
}
#else
#error "Unsupported Euzebia3D audio player platform"
#endif

static void init_audio_player(const IHardware *hardware)
{
    _hardware = hardware;
    audio_player_init();
}

static IAudioPlayer audioPlayer = {
    .init_audio_player = init_audio_player,
    .play_wave_file = play_wave_file,
};

const IAudioPlayer *get_audioPlayer(void)
{
    return &audioPlayer;
}
