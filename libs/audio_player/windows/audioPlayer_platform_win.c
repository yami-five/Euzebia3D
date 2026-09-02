#include "audioPlayer_platform.h"

#include <stddef.h>

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

void audio_player_init(const e3d_IHardware *hardware) {
  (void)hardware;

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

void audio_player_play_wave_file(char *file_name) {
  if (file_name == NULL || file_name[0] == '\0') {
    return;
  }

  if (!audio_engine_initialized) {
    audio_player_init(NULL);
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

bool audio_player_is_storage_ready(void) {
  return audio_engine_initialized != 0;
}
