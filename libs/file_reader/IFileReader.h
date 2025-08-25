#ifndef IFILEREADER_h
#define IFILEREADER_h

#include <stdint.h>
#include "IPainter.h"
#include "IHardware.h"

typedef struct
{
    void (*init_fileReader)(const IHardware *hardware);
    void (*play_wave_file)(char *file_name);
} IFileReader;

#endif