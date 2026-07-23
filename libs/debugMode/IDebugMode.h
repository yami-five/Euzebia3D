#ifndef IDEBUGMODE_h
#define IDEBUGMODE_h

#include "IHardware.h"
#include "IPainter.h"
#include <stdint.h>

typedef struct {
  void (*init_debug_mode)(const e3d_IHardware *hardware, const e3d_IPainter *painter);
  void (*begin_frame)(void);
  void (*begin_draw_buffer)(void);
  void (*end_draw_buffer)(void);
  void (*end_frame)(void);
  void (*reset_window)(void);
  void (*show_info)(void);
} e3d_IDebugMode;

#endif
