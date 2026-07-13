#ifndef IAUDIOPLAYER_H
#define IAUDIOPLAYER_H

#include <stdint.h>
#include "IHardware.h"

typedef struct
{
    void (*init_audio_player)(const IHardware *hardware);
    void (*play_wave_file)(char *file_name);
} IAudioPlayer;

#endif
