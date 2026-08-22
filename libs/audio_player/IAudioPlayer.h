#ifndef IAUDIOPLAYER_H
#define IAUDIOPLAYER_H

#include "IHardware.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  void (*init_audio_player)(const e3d_IHardware *hardware);
  void (*play_wave_file)(char *file_name);
  bool (*is_storage_ready)(void);
} e3d_IAudioPlayer;

#endif
