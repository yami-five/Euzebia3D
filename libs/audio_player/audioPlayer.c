#include "audioPlayer.h"
#include "IAudioPlayer.h"

#include <stdbool.h>
#include <stddef.h>

static const e3d_IHardware *_hardware = NULL;

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

static void stop_current_sound(void) {
  if (!current_sound_initialized) {
    return;
  }

  (void)ma_sound_stop(&current_sound);
  ma_sound_uninit(&current_sound);
  current_sound_initialized = 0;
}

static void audio_player_shutdown(void) {
  stop_current_sound();

  if (!audio_engine_initialized) {
    return;
  }

  ma_engine_uninit(&audio_engine);
  audio_engine_initialized = 0;
}

static void audio_player_init(void) {
  if (audio_engine_initialized) {
    return;
  }

  ma_result result = ma_engine_init(NULL, &audio_engine);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "miniaudio: failed to initialize engine (%d)\n", result);
    return;
  }

  audio_engine_initialized = 1;

  if (!audio_player_shutdown_registered && atexit(audio_player_shutdown) == 0) {
    audio_player_shutdown_registered = 1;
  }
}

static void play_wave_file(char *file_name) {
  if (file_name == NULL || file_name[0] == '\0') {
    return;
  }

  if (!audio_engine_initialized) {
    audio_player_init();
  }

  if (!audio_engine_initialized) {
    return;
  }

  stop_current_sound();

  ma_result result = ma_sound_init_from_file(&audio_engine, file_name, 0, NULL,
                                             NULL, &current_sound);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "miniaudio: failed to load '%s' (%d)\n", file_name, result);
    return;
  }

  current_sound_initialized = 1;

  result = ma_sound_start(&current_sound);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "miniaudio: failed to play '%s' (%d)\n", file_name, result);
    stop_current_sound();
  }
}
static bool is_storage_ready(void) { return audio_engine_initialized != 0; }
#elif defined(EUZEBIA3D_PLATFORM_PICO)
#include "../storage/pins.h"
#include "fatfs/ff.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/sync/spin_lock.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "string.h"

static FRESULT f_res;
static FATFS microSDFatFs;
static bool sd_storage_ready = false;

typedef struct {
  uint32_t sample_rate;
  uint32_t data_size;
  uint16_t channel_count;
  uint16_t bits_per_sample;
} wav_info_t;

static uint16_t read_le16(const uint8_t *data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static bool read_exact(FIL *file, void *buffer, UINT bytes_to_read) {
  UINT bytes_read = 0;
  return f_read(file, buffer, bytes_to_read, &bytes_read) == FR_OK &&
         bytes_read == bytes_to_read;
}

static bool skip_bytes(FIL *file, uint32_t bytes_to_skip) {
  FSIZE_t current_position = f_tell(file);
  FSIZE_t target_position = current_position + (FSIZE_t)bytes_to_skip;

  if (target_position < current_position || target_position > f_size(file))
    return false;

  return f_lseek(file, target_position) == FR_OK;
}

static bool read_wav_info(FIL *file, wav_info_t *wav_info) {
  uint8_t riff_header[12];
  uint8_t chunk_header[8];
  bool format_found = false;

  if (!read_exact(file, riff_header, sizeof(riff_header)) ||
      memcmp(riff_header, "RIFF", 4) != 0 ||
      memcmp(riff_header + 8, "WAVE", 4) != 0)
    return false;

  while (f_tell(file) + sizeof(chunk_header) <= f_size(file)) {
    if (!read_exact(file, chunk_header, sizeof(chunk_header)))
      return false;

    uint32_t chunk_size = read_le32(chunk_header + 4);

    if (memcmp(chunk_header, "fmt ", 4) == 0) {
      uint8_t format_data[16];
      if (chunk_size < sizeof(format_data) ||
          !read_exact(file, format_data, sizeof(format_data)))
        return false;

      uint16_t audio_format = read_le16(format_data);
      wav_info->channel_count = read_le16(format_data + 2);
      wav_info->sample_rate = read_le32(format_data + 4);
      wav_info->bits_per_sample = read_le16(format_data + 14);

      if (audio_format != 1 ||
          (wav_info->channel_count != 1 && wav_info->channel_count != 2) ||
          wav_info->sample_rate == 0 || wav_info->bits_per_sample != 16)
        return false;

      uint32_t remaining_format_bytes =
          chunk_size - (uint32_t)sizeof(format_data);
      if (!skip_bytes(file, remaining_format_bytes + (chunk_size & 1u)))
        return false;

      format_found = true;
    } else if (memcmp(chunk_header, "data", 4) == 0) {
      if (!format_found || chunk_size > f_size(file) - f_tell(file))
        return false;

      wav_info->data_size = chunk_size;
      return true;
    } else if (!skip_bytes(file, chunk_size + (chunk_size & 1u))) {
      return false;
    }
  }

  return false;
}

static void audio_player_init(void) {
  _hardware->write(SD_CS_PIN, 1);

  // Check the mounted device.
  f_res = f_mount(&microSDFatFs, (TCHAR const *)"/", 1);
  sd_storage_ready = (f_res == FR_OK);
  if (f_res != FR_OK)
    printf("SD card mount file system failed ,error code :(%d)\r\n", f_res);
  else
    printf("SD card mount file system success!! \r\n");
}

static void play_wave_file(char *file_name) {
  if (file_name == NULL || file_name[0] == '\0')
    return;

  FIL file;
  wav_info_t wav_info;

  f_res = f_open(&file, file_name, FA_READ);
  if (f_res != FR_OK) {
    printf("Loading file failed :(%d)\r\n", f_res);
    return;
  }

  if (!read_wav_info(&file, &wav_info)) {
    printf("Unsupported or invalid WAV file: %s\r\n", file_name);
    f_close(&file);
    return;
  }

  _hardware->init_audio_i2s(wav_info.sample_rate, wav_info.channel_count);

  struct audio_buffer_pool *audio_buffer_pool =
      _hardware->get_audio_buffer_pool();
  if (audio_buffer_pool == NULL) {
    printf("Audio I2S initialization failed\r\n");
    f_close(&file);
    return;
  }

  printf("Playing %s: %lu Hz, %u channel(s), %u-bit\r\n", file_name,
         (unsigned long)wav_info.sample_rate, wav_info.channel_count,
         wav_info.bits_per_sample);

  const uint32_t bytes_per_frame =
      wav_info.channel_count * (wav_info.bits_per_sample / 8u);
  uint8_t buffer_audio[256];
  uint32_t bytes_remaining = wav_info.data_size;

  while (bytes_remaining >= bytes_per_frame) {
    UINT bytes_to_read =
        bytes_remaining < sizeof(buffer_audio) ? (UINT)bytes_remaining
                                               : (UINT)sizeof(buffer_audio);
    bytes_to_read -= bytes_to_read % bytes_per_frame;

    UINT bytes_read = 0;
    f_res = f_read(&file, buffer_audio, bytes_to_read, &bytes_read);
    if (f_res != FR_OK || bytes_read == 0)
      break;

    UINT complete_frame_bytes = bytes_read - (bytes_read % bytes_per_frame);
    if (complete_frame_bytes == 0)
      break;

    struct audio_buffer *audio_buf =
        take_audio_buffer(audio_buffer_pool, true);
    memcpy(audio_buf->buffer->bytes, buffer_audio, complete_frame_bytes);
    audio_buf->sample_count = complete_frame_bytes / bytes_per_frame;
    give_audio_buffer(audio_buffer_pool, audio_buf);

    bytes_remaining -= bytes_read;
  }

  if (f_res != FR_OK)
    printf("Audio file read failed: (%d)\r\n", f_res);

  f_close(&file);
}
static bool is_storage_ready(void) { return sd_storage_ready; }
#else
#error "Unsupported Euzebia3D audio player platform"
#endif

static void init_audio_player(const e3d_IHardware *hardware) {
  _hardware = hardware;
  audio_player_init();
}

static e3d_IAudioPlayer audioPlayer = {
    .init_audio_player = init_audio_player,
    .play_wave_file = play_wave_file,
    .is_storage_ready = is_storage_ready,
};

const e3d_IAudioPlayer *get_audioPlayer(void) { return &audioPlayer; }
