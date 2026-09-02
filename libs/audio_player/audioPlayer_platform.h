#ifndef AUDIO_PLAYER_PLATFORM_H
#define AUDIO_PLAYER_PLATFORM_H

#include "IAudioPlayer.h"

void audio_player_init(const e3d_IHardware *hardware);
void audio_player_play_wave_file(char *file_name);
bool audio_player_is_storage_ready(void);

#endif
