#include "IDebugMode.h"
#include "debugMode.h"
#include <stddef.h>
#include <stdio.h>

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#elif defined(EUZEBIA3D_PLATFORM_PICO)
#include "pico/time.h"
#endif

#define DEBUG_MODE_UPDATE_WINDOW_US 500000ull

typedef struct
{
    uint64_t tick_frequency;
    uint64_t frame_begin_ticks;
    uint64_t draw_buffer_begin_ticks;
    uint64_t frame_draw_buffer_ticks;
    uint64_t window_us;
    uint64_t rest_window_us;
    uint64_t draw_buffer_window_us;
    uint32_t window_frames;
    uint32_t lcd_spi_baudrate_hz;
    uint8_t frame_active;
    uint8_t draw_buffer_active;
    char text[80];
} DebugModeState;

static const IHardware *_hardware = NULL;
static const IPainter *_painter = NULL;
static DebugModeState state;

static uint64_t get_debug_ticks(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return SDL_GetPerformanceCounter();
#elif defined(EUZEBIA3D_PLATFORM_PICO)
    return time_us_64();
#else
    return 0u;
#endif
}

static uint64_t get_debug_tick_frequency(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    return SDL_GetPerformanceFrequency();
#elif defined(EUZEBIA3D_PLATFORM_PICO)
    return 1000000u;
#else
    return 0u;
#endif
}

static uint64_t debug_ticks_to_us(uint64_t ticks)
{
    if (state.tick_frequency == 0u)
        return 0u;

    return (ticks * 1000000ull + (state.tick_frequency >> 1)) / state.tick_frequency;
}

static void format_debug_text(uint32_t fps_x10, uint32_t rest_ms_x10, uint32_t draw_buffer_ms_x10)
{
    if (state.lcd_spi_baudrate_hz > 0u)
    {
        uint32_t spi_mhz_x10 = (state.lcd_spi_baudrate_hz + 50000u) / 100000u;
        snprintf(
            state.text,
            sizeof(state.text),
            "FPS: %lu.%lu\nREST: %lu.%lu  DRAW: %lu.%lu\nSPI: %lu.%lu MHz",
            (unsigned long)(fps_x10 / 10u),
            (unsigned long)(fps_x10 % 10u),
            (unsigned long)(rest_ms_x10 / 10u),
            (unsigned long)(rest_ms_x10 % 10u),
            (unsigned long)(draw_buffer_ms_x10 / 10u),
            (unsigned long)(draw_buffer_ms_x10 % 10u),
            (unsigned long)(spi_mhz_x10 / 10u),
            (unsigned long)(spi_mhz_x10 % 10u));
        return;
    }

    snprintf(
        state.text,
        sizeof(state.text),
        "FPS: %lu.%lu\nREST: %lu.%lu  DRAW: %lu.%lu",
        (unsigned long)(fps_x10 / 10u),
        (unsigned long)(fps_x10 % 10u),
        (unsigned long)(rest_ms_x10 / 10u),
        (unsigned long)(rest_ms_x10 % 10u),
        (unsigned long)(draw_buffer_ms_x10 / 10u),
        (unsigned long)(draw_buffer_ms_x10 % 10u));
}

static void reset_debug_mode_window(void)
{
    state.window_us = 0u;
    state.rest_window_us = 0u;
    state.draw_buffer_window_us = 0u;
    state.window_frames = 0u;
    format_debug_text(0u, 0u, 0u);
}

static void update_debug_mode(uint64_t rest_ticks, uint64_t draw_buffer_ticks)
{
    if (state.tick_frequency == 0u || (rest_ticks == 0u && draw_buffer_ticks == 0u))
        return;

    uint64_t rest_us = debug_ticks_to_us(rest_ticks);
    uint64_t draw_buffer_us = debug_ticks_to_us(draw_buffer_ticks);
    uint64_t frame_us = rest_us + draw_buffer_us;
    if (frame_us == 0u)
        return;

    state.window_us += frame_us;
    state.rest_window_us += rest_us;
    state.draw_buffer_window_us += draw_buffer_us;
    state.window_frames++;

    if (state.window_us >= DEBUG_MODE_UPDATE_WINDOW_US)
    {
        uint32_t avg_frame_us = (uint32_t)((state.window_us + (state.window_frames >> 1)) / state.window_frames);
        uint32_t avg_rest_us = (uint32_t)((state.rest_window_us + (state.window_frames >> 1)) / state.window_frames);
        uint32_t avg_draw_buffer_us = (uint32_t)((state.draw_buffer_window_us + (state.window_frames >> 1)) / state.window_frames);
        uint32_t fps_x10 = 0u;
        uint32_t rest_ms_x10 = 0u;
        uint32_t draw_buffer_ms_x10 = 0u;

        if (avg_frame_us > 0u)
        {
            fps_x10 = (uint32_t)((10000000ull + ((uint64_t)avg_frame_us >> 1)) / (uint64_t)avg_frame_us);
            rest_ms_x10 = (uint32_t)(((uint64_t)avg_rest_us + 50ull) / 100ull);
            draw_buffer_ms_x10 = (uint32_t)(((uint64_t)avg_draw_buffer_us + 50ull) / 100ull);
        }

        format_debug_text(fps_x10, rest_ms_x10, draw_buffer_ms_x10);

        state.window_us = 0u;
        state.rest_window_us = 0u;
        state.draw_buffer_window_us = 0u;
        state.window_frames = 0u;
    }
}

static void init_debug_mode(const IHardware *hardware, const IPainter *painter)
{
    _hardware = hardware;
    _painter = painter;
    state.tick_frequency = get_debug_tick_frequency();
    state.frame_begin_ticks = 0u;
    state.draw_buffer_begin_ticks = 0u;
    state.frame_draw_buffer_ticks = 0u;
    state.frame_active = 0u;
    state.draw_buffer_active = 0u;
    state.lcd_spi_baudrate_hz = 0u;

    if (_hardware != NULL && _hardware->get_lcd_spi_baudrate_hz != NULL)
        state.lcd_spi_baudrate_hz = _hardware->get_lcd_spi_baudrate_hz();

    reset_debug_mode_window();
}

static void begin_frame(void)
{
    state.frame_begin_ticks = get_debug_ticks();
    state.draw_buffer_begin_ticks = 0u;
    state.frame_draw_buffer_ticks = 0u;
    state.frame_active = 1u;
    state.draw_buffer_active = 0u;
}

static void begin_draw_buffer(void)
{
    if (!state.frame_active)
        begin_frame();

    state.draw_buffer_begin_ticks = get_debug_ticks();
    state.draw_buffer_active = 1u;
}

static void end_draw_buffer(void)
{
    if (!state.draw_buffer_active)
        return;

    uint64_t draw_buffer_end_ticks = get_debug_ticks();
    state.frame_draw_buffer_ticks += draw_buffer_end_ticks - state.draw_buffer_begin_ticks;
    state.draw_buffer_active = 0u;
}

static void end_frame(void)
{
    if (!state.frame_active)
        return;

    if (state.draw_buffer_active)
        end_draw_buffer();

    uint64_t frame_end_ticks = get_debug_ticks();
    uint64_t frame_ticks = frame_end_ticks - state.frame_begin_ticks;
    uint64_t rest_ticks = 0u;
    if (frame_ticks > state.frame_draw_buffer_ticks)
        rest_ticks = frame_ticks - state.frame_draw_buffer_ticks;

    update_debug_mode(rest_ticks, state.frame_draw_buffer_ticks);
    state.frame_active = 0u;
}

static void show_info(void)
{
    if (_painter == NULL)
        return;

    _painter->print(state.text, 4, 4, 0, 0xffff);
}

static IDebugMode debugMode = {
    .init_debug_mode = init_debug_mode,
    .begin_frame = begin_frame,
    .begin_draw_buffer = begin_draw_buffer,
    .end_draw_buffer = end_draw_buffer,
    .end_frame = end_frame,
    .reset_window = reset_debug_mode_window,
    .show_info = show_info,
};

const IDebugMode *get_debugMode(void)
{
    return &debugMode;
}
