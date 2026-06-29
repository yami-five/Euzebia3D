#ifndef IDEBUGMODE_h
#define IDEBUGMODE_h

#include <stdint.h>
#include "IHardware.h"
#include "IPainter.h"

typedef struct
{
    void(*init_debug_mode)(const IHardware *hardware, const IPainter *painter);
    void(*begin_frame)(void);
    void(*begin_draw_buffer)(void);
    void(*end_draw_buffer)(void);
    void(*end_frame)(void);
    void(*reset_window)(void);
    void(*show_info)(void);
} IDebugMode;

#endif
