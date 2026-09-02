#include "audioPlayer.h"
#include "audioPlayer_platform.h"

static const e3d_IAudioPlayer audioPlayer = {
    .init_audio_player = audio_player_init,
    .play_wave_file = audio_player_play_wave_file,
    .is_storage_ready = audio_player_is_storage_ready,
};

const e3d_IAudioPlayer *get_audioPlayer(void) { return &audioPlayer; }
