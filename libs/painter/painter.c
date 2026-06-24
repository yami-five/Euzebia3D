#include "IPainter.h"
#include "fpa.h"
#include <string.h>
#include <stdlib.h>

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include <SDL3/SDL.h>
#else
#include "painter.h"
#include "hardware/sync/spin_lock.h"
#endif

#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 320
#endif

#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 240
#endif

#ifndef WIDTH_HALF
#define WIDTH_HALF 160
#endif

#ifndef HEIGHT_HALF
#define HEIGHT_HALF 120
#endif

#ifndef BUFFER_SIZE_HALF
#define BUFFER_SIZE_HALF ((uint32_t)DISPLAY_WIDTH * (uint32_t)DISPLAY_HEIGHT)
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (BUFFER_SIZE_HALF * sizeof(uint16_t))
#endif

#define FONT_ASCII_FIRST 33u
#define FONT_ASCII_LAST 125u
#define FONT_GLYPHS_COUNT ((FONT_ASCII_LAST - FONT_ASCII_FIRST) + 1u)

typedef struct
{
    const uint16_t *pixels;
    uint8_t height;
    uint8_t width;
    bool canRotate;
} CachedSprite;

_Static_assert(sizeof(CachedSprite) == sizeof(Sprite), "CachedSprite must match Sprite layout");
_Static_assert(offsetof(CachedSprite, pixels) == offsetof(Sprite, pixels), "CachedSprite::pixels offset mismatch");
_Static_assert(offsetof(CachedSprite, height) == offsetof(Sprite, height), "CachedSprite::height offset mismatch");
_Static_assert(offsetof(CachedSprite, width) == offsetof(Sprite, width), "CachedSprite::width offset mismatch");
_Static_assert(offsetof(CachedSprite, canRotate) == offsetof(Sprite, canRotate), "CachedSprite::canRotate offset mismatch");

static const IHardware *_hardware = NULL;
static const IDisplay *_display = NULL;
static const IStorage *_storage = NULL;
static uint16_t buffer[BUFFER_SIZE_HALF];
static uint16_t temp_buffer[BUFFER_SIZE_HALF];
static uint16_t *font_cache_pixels[FONT_GLYPHS_COUNT];
static CachedSprite font_cache_sprites[FONT_GLYPHS_COUNT];
static uint8_t font_cache_widths[FONT_GLYPHS_COUNT];
static uint8_t font_cache_ready = 0;
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
static uint16_t mirrored_buffer[BUFFER_SIZE_HALF];
static SDL_Window *sdl_window = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture *sdl_texture = NULL;
static uint8_t sdl_video_initialized_here = 0;
static uint8_t sdl_cleanup_registered = 0;
#else
static const uint32_t chunk_size = 15360;
static spin_lock_t *lcd_spinlock;
#endif
static uint8_t scanline_offset = 0;
static const uint8_t DEFAULT_FONT_SIZE = 8;

static inline uint32_t pixel_index(uint16_t x, uint16_t y)
{
    return ((uint32_t)y * DISPLAY_WIDTH) + x;
}

static void init_font_cache(void)
{
    if (font_cache_ready || _storage == NULL || _storage->get_font_by_index == NULL)
        return;

    for (uint8_t i = 0; i < FONT_GLYPHS_COUNT; i++)
    {
        const Font *source_font = _storage->get_font_by_index(i);
        if (source_font == NULL || source_font->sprite == NULL || source_font->sprite->pixels == NULL)
            goto cache_fail;

        const Sprite *source_sprite = source_font->sprite;
        size_t pixel_count = (size_t)source_sprite->width * (size_t)source_sprite->height;
        uint16_t *pixels = (uint16_t *)malloc(pixel_count * sizeof(uint16_t));
        if (pixels == NULL)
            goto cache_fail;

        if (source_sprite->width == source_sprite->height)
        {
            for (uint16_t y = 0; y < source_sprite->height; y++)
            {
                for (uint16_t x = 0; x < source_sprite->width; x++)
                {
                    size_t dst = ((size_t)y * source_sprite->width) + x;
                    size_t src = ((size_t)x * source_sprite->width) + (source_sprite->width - 1u - y);
                    pixels[dst] = source_sprite->pixels[src];
                }
            }
        }
        else
        {
            // Non-square glyphs are copied as-is to keep indexing valid.
            memcpy(pixels, source_sprite->pixels, pixel_count * sizeof(uint16_t));
        }

        font_cache_pixels[i] = pixels;
        font_cache_sprites[i].canRotate = false;
        font_cache_sprites[i].height = source_sprite->height;
        font_cache_sprites[i].width = source_sprite->width;
        font_cache_sprites[i].pixels = pixels;
        font_cache_widths[i] = source_font->width;
    }

    font_cache_ready = 1;
    return;

cache_fail:
    for (uint8_t j = 0; j < FONT_GLYPHS_COUNT; j++)
    {
        if (font_cache_pixels[j] != NULL)
        {
            free(font_cache_pixels[j]);
            font_cache_pixels[j] = NULL;
        }
    }
}

static const uint8_t fadeInPatterns[9][16] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1},
    {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

static const uint8_t fadeOutPatterns[9][16] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
    {1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
static void destroy_sdl_backend(void)
{
    if (sdl_texture != NULL)
    {
        SDL_DestroyTexture(sdl_texture);
        sdl_texture = NULL;
    }
    if (sdl_renderer != NULL)
    {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
    }
    if (sdl_window != NULL)
    {
        SDL_DestroyWindow(sdl_window);
        sdl_window = NULL;
    }
    if (sdl_video_initialized_here)
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        sdl_video_initialized_here = 0;
    }
}

static int ensure_sdl_backend(void)
{
    if (sdl_texture != NULL && sdl_renderer != NULL && sdl_window != NULL)
        return 1;

    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0)
    {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            SDL_Log("Painter SDL init failed: %s", SDL_GetError());
            return 0;
        }
        sdl_video_initialized_here = 1;
    }

    sdl_window = SDL_CreateWindow(
        "Euzebia3D",
        DISPLAY_WIDTH * 3,
        DISPLAY_HEIGHT * 3,
        SDL_WINDOW_RESIZABLE);
    if (sdl_window == NULL)
    {
        SDL_Log("Painter SDL window creation failed: %s", SDL_GetError());
        destroy_sdl_backend();
        return 0;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
    if (sdl_renderer == NULL)
    {
        SDL_Log("Painter SDL renderer creation failed: %s", SDL_GetError());
        destroy_sdl_backend();
        return 0;
    }

    sdl_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT);
    if (sdl_texture == NULL)
    {
        SDL_Log("Painter SDL texture creation failed: %s", SDL_GetError());
        destroy_sdl_backend();
        return 0;
    }

    SDL_SetRenderLogicalPresentation(
        sdl_renderer,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        SDL_LOGICAL_PRESENTATION_LETTERBOX);

    if (!sdl_cleanup_registered)
    {
        atexit(destroy_sdl_backend);
        sdl_cleanup_registered = 1;
    }
    return 1;
}
#else
static void dma_buffer_irq_handler(void)
{
    dma_hw->ints1 = 1u << dma_channel;
}

static void init_dma(void)
{
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_dreq(&config, spi_get_dreq(_hardware->get_spi_port(), true));
    dma_channel_configure(
        dma_channel,
        &config,
        &spi_get_hw(_hardware->get_spi_port())->dr,
        NULL,
        chunk_size,
        false);
    dma_channel_set_irq1_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_buffer_irq_handler);
    irq_set_enabled(DMA_IRQ_1, false);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
}
#endif

void init_painter(const IDisplay *display, const IHardware *hardware, const IStorage *storage)
{
    _hardware = hardware;
    _display = display;
    _storage = storage;
    init_font_cache();

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    (void)_hardware;
    (void)_display;
    ensure_sdl_backend();
#else
    init_dma();
    _hardware->write(SD_CS_PIN, 1);
    _hardware->write(LCD_CS_PIN, 0);
    (void)lcd_spinlock;
#endif
}

void draw_buffer(void)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (!ensure_sdl_backend())
        return;

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++)
    {
        uint32_t dst_row_start = (uint32_t)y * DISPLAY_WIDTH;
        uint32_t src_row_start = (uint32_t)(DISPLAY_HEIGHT - 1 - y) * DISPLAY_WIDTH;
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++)
            mirrored_buffer[dst_row_start + x] = buffer[src_row_start + x];
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            break;
    }

    SDL_UpdateTexture(
        sdl_texture,
        NULL,
        mirrored_buffer,
        DISPLAY_WIDTH * (int32_t)sizeof(uint16_t));
    SDL_RenderClear(sdl_renderer);
    SDL_RenderTexture(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
#else
    uint32_t current_offset = 0;
    spi_inst_t *spi_port = _hardware->get_spi_port();
    spin_lock_t *spi_spinlock = _hardware->get_spinlock();
    (void)spi_spinlock;

    spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    _hardware->write(LCD_DC_PIN, 0);
    _hardware->spi_write_byte(0x2C);
    _hardware->write(LCD_DC_PIN, 1);
    spi_set_format(spi_port, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    while (current_offset < BUFFER_SIZE_HALF)
    {
        dma_channel_set_read_addr(dma_channel, buffer + current_offset, true);
        dma_channel_wait_for_finish_blocking(dma_channel);
        current_offset += chunk_size;
    }

    while (spi_is_busy(spi_port))
    {
    }

    spi_set_format(spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
#endif
}

void clear_buffer(uint16_t color)
{
    for (uint32_t i = 0; i < BUFFER_SIZE_HALF; i++)
        buffer[i] = color;
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
        return;
    uint32_t line_adr = pixel_index(x, y);
    buffer[line_adr] = color;
}

void draw_span(uint16_t x, uint16_t y, const uint16_t *span, uint16_t span_length)
{
    if (span == NULL || span_length == 0 || y >= DISPLAY_HEIGHT || x >= DISPLAY_WIDTH)
        return;
    if ((uint32_t)x + span_length > DISPLAY_WIDTH)
        span_length = DISPLAY_WIDTH - x;

    memcpy(&buffer[pixel_index(x, y)], span, span_length * sizeof(uint16_t));
}

void draw_image(uint8_t image_index)
{
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    if (_storage == NULL)
        return;

    const Image *image = _storage->get_image(image_index);
    if (image == NULL || image->image == NULL)
        return;

    memcpy(buffer, image->image, BUFFER_SIZE);
#else
    int dma_channel_flash = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_channel_flash);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, true);
    dma_channel_configure(
        dma_channel_flash,
        &config,
        buffer,
        _storage->get_image(image_index)->image,
        BUFFER_SIZE_HALF,
        false);
    dma_channel_start(dma_channel_flash);
    dma_channel_wait_for_finish_blocking(dma_channel_flash);
    dma_channel_unclaim(dma_channel_flash);
#endif
}

static inline uint8_t get_r(uint16_t c) { return (c >> 11) & 0x1F; }
static inline uint8_t get_g(uint16_t c) { return (c >> 5) & 0x3F; }
static inline uint8_t get_b(uint16_t c) { return c & 0x1F; }

static inline uint16_t make_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

void crt_disp_effect()
{
    // barrel distortion
    uint16_t *framebuffer = (uint16_t *)malloc(sizeof(uint16_t) * BUFFER_SIZE_HALF);
    if (framebuffer == NULL)
        return;
    // for (uint32_t i = 0; i < BUFFER_SIZE_HALF; i++)
    // {
    //     uint32_t index = get_effect_table_element(0, i);
    //     framebuffer[i * 2] = buffer[index * 2];
    //     framebuffer[i * 2 + 1] = buffer[index * 2 + 1];
    // }
    // memcpy(buffer, framebuffer, BUFFER_SIZE);
    // chromatic aberration
    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++)
    {
        uint32_t ydw = (uint32_t)y * DISPLAY_WIDTH;
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint32_t i = ydw + x;

            uint16_t xr = (x > 1) ? (uint16_t)(x - 2) : x;
            uint16_t xg = (x + 2 < DISPLAY_WIDTH) ? (uint16_t)(x + 2) : x;

            uint32_t ir = ydw + xr;
            uint32_t ig = ydw + xg;

            uint16_t c_r = buffer[ir];
            uint16_t c_g = buffer[ig];
            uint16_t c_b = buffer[i];

            uint8_t r = get_r(c_r);
            uint8_t g = get_g(c_g);
            uint8_t b = get_b(c_b);
            uint16_t result = make_rgb565(r, g, b);

            framebuffer[i] = result;
        }
    }
    memcpy(buffer, framebuffer, BUFFER_SIZE);
    free(framebuffer);
    // scanline
    // for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++)
    // {
    //     for (uint16_t x = scanline_offset; x < DISPLAY_WIDTH; x += 4)
    //     {
    //         draw_pixel(x, y, 0);
    //     }
    // }
    // scanline_offset = !scanline_offset;
}

void fake_glow_effect(uint16_t *params)
{
    if (params == NULL)
        return;
    uint16_t r = params[0];
    const uint16_t MAX_GLOW_RADIUS = 40;
    if (r > MAX_GLOW_RADIUS)
        r = MAX_GLOW_RADIUS;
    uint16_t mainColor = params[1];
    uint16_t maxOffsets = ((r << 1) + 1);
    maxOffsets *= maxOffsets;
    int *offsets = (int *)malloc(sizeof(int) * (maxOffsets << 1));
    if (offsets == NULL)
        return;
    uint16_t offsetsNum = 0;
    uint16_t r2 = r * r;
    for (int16_t dy = -r; dy <= r; dy++)
    {
        for (int16_t dx = -r; dx <= r; dx++)
        {
            uint16_t dist = dx * dx + dy * dy;
            if (dist <= r2)
            {
                offsets[offsetsNum] = dy * DISPLAY_WIDTH + dx;
                offsetsNum++;
                offsets[offsetsNum] = dist;
                offsetsNum++;
            }
        }
    }
    for (uint32_t i = 0; i < BUFFER_SIZE_HALF; i++)
    {
        uint16_t color = buffer[i];
        if (color != mainColor)
            continue;
        else
        {
            for (uint16_t j = 0; j < offsetsNum; j += 2)
            {
                int addr = i + offsets[j];
                if (addr < 0 || addr >= BUFFER_SIZE_HALF)
                    continue;
                uint16_t r5 = ((r2 - offsets[j + 1]) * 31 + (r2 >> 1)) / r2;
                uint16_t g6 = ((r2 - offsets[j + 1]) * 63 + (r2 >> 1)) / r2;
                r5 = (r5 * 200 + 127) / 255;
                g6 = (g6 * 200 + 127) / 255;
                uint16_t new_color = (r5 << 11) | (g6 << 5) | r5;
                if (new_color > buffer[addr])
                    buffer[addr] = new_color;
            }
        }
    }
    free(offsets);
}

void blue_only()
{
    for (uint32_t i = 0; i < BUFFER_SIZE_HALF; i++)
    {
        uint16_t color = buffer[i];
        if (color == 0)
            continue;
        uint8_t b = get_b(color);
        color = make_rgb565(3, 5, b);
        buffer[i] = color;
    }
}

void broken_chromatic_abberration()
{
    for (int y = 0; y < DISPLAY_HEIGHT; y++)
    {
        uint16_t yr = (y > 0) ? y - 2 : y;
        uint16_t yg = (y < DISPLAY_HEIGHT - 1) ? y + 2 : y;
        for (int x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint32_t i = y * DISPLAY_WIDTH + x;

            uint16_t xr = (x > 0) ? x - 5 : x;
            uint16_t xg = (x < DISPLAY_WIDTH - 1) ? x + 5 : x;

            uint16_t ir = yr * DISPLAY_WIDTH + xr;
            uint16_t ig = yg * DISPLAY_WIDTH + xg;

            uint16_t c_r = buffer[ir];
            uint16_t c_g = buffer[ig];
            uint16_t c_b = buffer[i];

            uint8_t r = get_r(c_r);
            uint8_t g = get_g(c_g);
            uint8_t b = get_b(c_b);
            uint16_t result = make_rgb565(r, g, b);

            temp_buffer[i] = result;
        }
    }
    memcpy(buffer, temp_buffer, BUFFER_SIZE);
}

void apply_post_process_effect(uint8_t effect_index, uint16_t *params)
{
    switch (effect_index)
    {
    case 0:
        crt_disp_effect();
        break;
    case 1:
        if (params != NULL)
            fake_glow_effect(params);
        break;
    case 2:
        blue_only();
        break;
    case 3:
        broken_chromatic_abberration();
        break;
    default:
        return;
    }
}

void middle_point(int16_t *x, int16_t *y, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    *x = x1 + ((x2 - x1) >> 1);
    *y = y1 + ((y2 - y1) >> 1);
}

static void draw_rectangle_size(const Sprite *sprite, int16_t pos_x, int16_t pos_y, uint8_t scale)
{
    if (sprite == NULL || sprite->pixels == NULL || scale == 0)
        return;

    int16_t sprite_height_scaled = sprite->height << (scale - 1);
    int16_t pos_y_aligned = (DISPLAY_HEIGHT - sprite_height_scaled) - pos_y;

    for (uint16_t y = 0; y < sprite->height; y++)
    {
        int16_t new_y = y + (pos_y_aligned >> (scale - 1));
        if (new_y < 0 || new_y >= DISPLAY_HEIGHT)
            continue;

        uint32_t row_offset = (uint32_t)y * sprite->width;
        for (uint16_t x = 0; x < sprite->width; x++)
        {
            uint16_t pixel = sprite->pixels[row_offset + x];
            if (pixel == 63519)
                continue;

            int16_t new_x = x + (pos_x >> (scale - 1));
            if (new_x < 0 || new_x >= DISPLAY_WIDTH)
                continue;

            for (uint8_t i = 0; i < scale; i++)
                for (uint8_t j = 0; j < scale; j++)
                    draw_pixel((new_x << (scale - 1)) + i, (new_y << (scale - 1)) + j, pixel);
        }
    }
}

void draw_sprite(const Sprite *sprite, int16_t pos_x, int16_t pos_y, int32_t angle, uint8_t scale)
{
    if (sprite == NULL || sprite->pixels == NULL || scale == 0)
        return;

    if (sprite->width != sprite->height)
    {
        draw_rectangle_size(sprite, pos_x, pos_y, scale);
        return;
    }

    uint8_t size = sprite->width;

    // Align sprite Y axis with the rest of renderer output.
    int16_t sprite_size_scaled = size << (scale - 1);
    int16_t pos_y_aligned = (DISPLAY_HEIGHT - sprite_size_scaled) - pos_y;

    if (angle != 0 && sprite->canRotate)
    {
        int16_t cos = fast_cos(angle);
        int16_t sin = fast_sin(angle);
        int8_t middle = size >> 1;
        for (uint16_t y = 0; y < size; y++)
        {
            int16_t new_y = y + (pos_y_aligned >> (scale - 1));
            if (new_y >= 0 && new_y < DISPLAY_HEIGHT)
            {
                for (uint16_t x = 0; x < size; x++)
                {
                    int16_t new_x = x + (pos_x >> (scale - 1));
                    int16_t xr = (((x - middle) * cos - (y - middle) * sin) >> SHIFT_FACTOR) + middle;
                    int16_t yr = (((x - middle) * sin + (y - middle) * cos) >> SHIFT_FACTOR) + middle;
                    if ((uint16_t)xr < size && (uint16_t)yr < size)
                    {
                        uint16_t pixel = sprite->pixels[((size_t)yr * size) + (size_t)xr];
                        if (pixel != 63519)
                        {
                            if (new_x >= 0 && new_x < DISPLAY_WIDTH)
                                for (uint8_t i = 0; i < scale; i++)
                                    for (uint8_t j = 0; j < scale; j++)
                                        draw_pixel((new_x * scale) + i, (new_y * scale) + j, pixel);
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (uint16_t y = 0; y < size; y++)
        {
            int16_t new_y = y + (pos_y_aligned >> (scale - 1));
            if (new_y >= 0 && new_y < DISPLAY_HEIGHT)
            {
                uint32_t ydh = (uint32_t)y * size;
                for (uint16_t x = 0; x < size; x++)
                {
                    uint16_t pixel = sprite->pixels[ydh + x];
                    if (pixel != 63519)
                    {
                        int16_t new_x = x + (pos_x >> (scale - 1));
                        if (new_x >= 0 && new_x < DISPLAY_WIDTH)
                            for (uint8_t i = 0; i < scale; i++)
                                for (uint8_t j = 0; j < scale; j++)
                                    draw_pixel((new_x << (scale - 1)) + i, (new_y << (scale - 1)) + j, pixel);
                    }
                }
            }
        }
    }
}

void draw_background(Image *image)
{
    if (image == NULL)
        return;
    if (image->image == NULL)
        return;
    if (image->width != DISPLAY_WIDTH || image->heigth != DISPLAY_HEIGHT)
        return;
    if (image->size != BUFFER_SIZE)
        return;

    memcpy(buffer, image->image, BUFFER_SIZE);
}

// Legacy font atlas is stored transposed (x/y swapped), so glyph sampling
// swaps coordinates to recover readable text on the current display layout.
static void draw_font_glyph_transposed(const Sprite *sprite, int16_t pos_x, int16_t pos_y, uint8_t scale)
{
    if (sprite == NULL || scale == 0)
        return;

    int16_t sprite_height_scaled = sprite->height << (scale - 1);
    int16_t pos_y_aligned = (DISPLAY_HEIGHT - sprite_height_scaled) - pos_y;
    bool square_sprite = sprite->width == sprite->height;

    for (uint16_t y = 0; y < sprite->height; y++)
    {
        int16_t new_y = y + (pos_y_aligned >> (scale - 1));
        if (new_y < 0 || new_y >= DISPLAY_HEIGHT)
            continue;

        for (uint16_t x = 0; x < sprite->width; x++)
        {
            uint16_t pixel = square_sprite
                                 ? sprite->pixels[((size_t)x * sprite->width) + (sprite->width - 1u - y)]
                                 : sprite->pixels[((size_t)y * sprite->width) + x];
            if (pixel == 63519)
                continue;

            int16_t new_x = x + (pos_x >> (scale - 1));
            if (new_x < 0 || new_x >= DISPLAY_WIDTH)
                continue;

            for (uint8_t i = 0; i < scale; i++)
                for (uint8_t j = 0; j < scale; j++)
                    draw_pixel((new_x << (scale - 1)) + i, (new_y << (scale - 1)) + j, pixel);
        }
    }
}

static void print(const char *text, int16_t x, int16_t y, uint8_t scale)
{
    if (scale == 0 || text == NULL || _storage == NULL)
        return;
    uint16_t offset = 0;
    uint16_t yFactor = 0;

    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == 32)
        {
            offset += (DEFAULT_FONT_SIZE << (scale - 1));
            continue;
        }
        else if (text[i] == 10)
        {
            yFactor += DEFAULT_FONT_SIZE << (scale);
            offset = 0;
            continue;
        }
        else if (text[i] == 9)
        {
            offset += DEFAULT_FONT_SIZE << (scale);
            continue;
        }

        uint8_t char_code = (uint8_t)text[i];
        if (char_code < FONT_ASCII_FIRST || char_code > FONT_ASCII_LAST)
        {
            offset += (DEFAULT_FONT_SIZE << (scale - 1));
            continue;
        }

        uint8_t glyph_index = (uint8_t)(char_code - FONT_ASCII_FIRST);
        if (font_cache_ready)
        {
            const Sprite *sprite = (const Sprite *)&font_cache_sprites[glyph_index];
            if (sprite->width != sprite->height)
                return;
            uint8_t width = font_cache_widths[glyph_index];
            draw_sprite(sprite, x + offset - (((sprite->width - width) << (scale - 1)) >> 1), y + yFactor, 0, scale);
            offset += (width << (scale - 1));
            continue;
        }

        const Font *font = _storage->get_font_by_index(glyph_index);
        if (font == NULL || font->sprite == NULL)
        {
            offset += (DEFAULT_FONT_SIZE << (scale - 1));
            continue;
        }
        draw_font_glyph_transposed(font->sprite, x + offset - (((font->sprite->width - font->width) << (scale - 1)) >> 1), y + yFactor, scale);
        offset += (font->width << (scale - 1));
    }
};

static inline uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r >> 3 << 11) | (g >> 2 << 5) | (b >> 3));
}

void draw_gradient(Gradient *gradient)
{
    GradientColor *calculatedGradient = (GradientColor *)malloc(sizeof(GradientColor) * 320);
    for (uint8_t i = 1; i < gradient->colorsNum; i++)
    {
        GradientColor *color1 = gradient->colors[i - 1];
        if (color1->pos >= 320)
            break;
        GradientColor *color2 = gradient->colors[i];
        uint16_t dist = color2->pos - color1->pos;
        if (dist == 0)
            continue;
        int16_t rDiff = color2->r - color1->r;
        int16_t gDiff = color2->g - color1->g;
        int16_t bDiff = color2->b - color1->b;
        for (uint16_t j = 0; j < dist; j++)
        {
            if (j + color1->pos >= 320)
                break;
            calculatedGradient[j + color1->pos].r = color1->r + rDiff * j / dist;
            calculatedGradient[j + color1->pos].g = color1->g + gDiff * j / dist;
            calculatedGradient[j + color1->pos].b = color1->b + bDiff * j / dist;
        }
    }
    for (uint16_t i = 0; i < 320; i++)
    {
        uint16_t color = rgb888_to_rgb565(calculatedGradient[i].r, calculatedGradient[i].g, calculatedGradient[i].b);
        for (uint16_t j = 0; j < 240; j++)
        {
            draw_pixel(i, j, color);
        }
    }
    free(calculatedGradient);
}

void override_buffer(uint8_t mode, uint16_t lines)
{
    if (lines > DISPLAY_HEIGHT)
        lines = DISPLAY_HEIGHT;
    if (mode == 0)
        memcpy(buffer, temp_buffer, lines * DISPLAY_WIDTH * sizeof(uint16_t));
    else if (mode == 1)
        memcpy(temp_buffer, buffer, lines * DISPLAY_WIDTH * sizeof(uint16_t));
}

void fade_fullscreen(uint8_t mode, uint32_t startFrame, uint32_t currentFrame)
{
    if (currentFrame < startFrame)
        return;
    uint8_t patternIndex = currentFrame - startFrame;
    if ((patternIndex == 0 && mode == 1) || (patternIndex == 8 && mode == 0))
        return;
    else if ((patternIndex == 0 && mode == 0) || (patternIndex == 8 && mode == 1))
    {
        clear_buffer(0);
        return;
    }
    uint8_t pattern[16];
    if (mode == 0) // fade out
    {
        memcpy(pattern, fadeOutPatterns[patternIndex], 16);
    }
    else // fade in
    {
        memcpy(pattern, fadeInPatterns[patternIndex], 16);
    }

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++)
    {
        uint8_t yMod4 = y & 3;
        uint32_t lineAddr = (uint32_t)y * DISPLAY_WIDTH;
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++)
        {
            uint32_t currentLineAddr = lineAddr + x;
            uint8_t patternAddr = (x & 3) << 2;
            if (pattern[patternAddr + yMod4] == 1)
                buffer[currentLineAddr] = 0;
        }
    }
}

void draw_scroller(const Scroller *scroller, uint16_t x, uint16_t y, uint32_t startFrame, uint32_t currentFrame)
{
    if (scroller == NULL || scroller->bitmap == NULL || scroller->width == 0 || scroller->height == 0)
        return;

    uint16_t square[2500];
    uint8_t squareWidth = 50;
    uint8_t squareHeight = 50;
    uint8_t line = currentFrame - startFrame;
    if (startFrame > currentFrame)
        line = 0;
    else if (line > squareHeight)
        line = squareHeight;
    memcpy(square, scroller->bitmap + line * scroller->width, sizeof(square));
    uint16_t lineBufferLen = (uint16_t)scroller->width << 1;
    uint16_t *lineBuffer = (uint16_t *)malloc((size_t)lineBufferLen * sizeof(uint16_t));
    if (lineBuffer == NULL)
        return;
    for (uint8_t i = 0; i < squareHeight; i++)
    {
        for (uint8_t j = 0; j < squareWidth; j++)
        {
            lineBuffer[j * 2] = square[i * squareWidth + j];
            lineBuffer[j * 2 + 1] = square[i * squareWidth + j];
        }
        uint32_t lineAddr1 = (uint32_t)(y + i * 2) * DISPLAY_WIDTH + x;
        uint32_t lineAddr2 = (uint32_t)(y + i * 2 + 1) * DISPLAY_WIDTH + x;
        memcpy(buffer + lineAddr1, lineBuffer, (size_t)lineBufferLen * sizeof(uint16_t));
        memcpy(buffer + lineAddr2, lineBuffer, (size_t)lineBufferLen * sizeof(uint16_t));
    }
    free(lineBuffer);
}

void fade(uint8_t mode, uint32_t startFrame, uint32_t currentFrame, uint16_t y, uint16_t x, uint16_t width, uint16_t height)
{
    if (currentFrame < startFrame)
        return;
    uint8_t patternIndex = currentFrame - startFrame;
    if (patternIndex > 8)
        return;
    if ((patternIndex == 8 && mode == 0))
        return;
    uint8_t pattern[16];

    if (mode == 0) // fade out
    {
        memcpy(pattern, fadeOutPatterns[patternIndex], 16);
    }
    else // fade in
    {
        memcpy(pattern, fadeInPatterns[patternIndex], 16);
    }

    uint16_t lineBufferSize = width * sizeof(uint16_t);
    uint16_t *lineBuffer = (uint16_t *)malloc(lineBufferSize);
    if (lineBuffer == NULL)
        return;

    for (uint8_t i = 0; i < height; i += 1)
    {
        uint32_t lineAddr = (uint32_t)(i + x) * DISPLAY_WIDTH;
        uint8_t patternAddr = (i & 3) << 2;
        for (uint8_t j = 0; j < width; j += 1)
        {
            uint32_t currentLineAddr = lineAddr + (y + j);
            uint8_t yMod4 = j & 3;
            if (pattern[patternAddr + yMod4] == 1)
                lineBuffer[j] = buffer[currentLineAddr];
            else
                lineBuffer[j] = temp_buffer[currentLineAddr];
        }
        lineAddr += y;
        memcpy(buffer + lineAddr, lineBuffer, lineBufferSize);
    }
    free(lineBuffer);
}



void draw_plasma(uint16_t *colors, uint8_t colorsNum, uint32_t t, int8_t facA, int8_t facB, int8_t facC, int8_t facD)
{
    int phase1 = (t << facA) % TABLE_SIZE;
    int phase2 = (t << facB) % TABLE_SIZE;
    int phase3 = (t << facC) % TABLE_SIZE;
    int phase4 = (t << facD) % TABLE_SIZE;

    int step_x    = TABLE_SIZE >> facA;
    int step_y    = TABLE_SIZE >> facB;
    int step_diag = TABLE_SIZE >> facC;
    int step_dist = TABLE_SIZE >> facD;

    uint16_t span[DISPLAY_WIDTH];
    for (int16_t x = 0; x < DISPLAY_HEIGHT; x++)
    {
        int32_t step1 = fast_sin(x * step_x + phase1) << 7;
        for (int16_t y = 0; y < DISPLAY_WIDTH; y++)
        {
            int dist = fast_sqrt((x - HEIGHT_HALF) * (x - HEIGHT_HALF) + (y - WIDTH_HALF) * (y- WIDTH_HALF));

            int32_t c = 0;
            c += step1;
            c += fast_sin(y * step_y + phase2)<<7;
            c += fast_sin((x + y) * step_diag + phase3) << 7;
            c += fast_sin(dist * step_dist + phase4);
            c >>= 4;
            span[y] = colors[((c>>SHIFT_FACTOR) + t) % colorsNum];
        }
        draw_span(0, x, span, DISPLAY_WIDTH);
    }
}

static IPainter painter = {
    .init_painter = init_painter,
    .draw_buffer = draw_buffer,
    .clear_buffer = clear_buffer,
    .draw_pixel = draw_pixel,
    .draw_span = draw_span,
    .draw_image = draw_image,
    .apply_post_process_effect = apply_post_process_effect,
    .draw_sprite = draw_sprite,
    .draw_background = draw_background,
    .print = print,
    .draw_gradient = draw_gradient,
    .override_buffer = override_buffer,
    .fade_fullscreen = fade_fullscreen,
    .draw_scroller = draw_scroller,
    .fade = fade,
    .draw_plasma = draw_plasma,
};

const IPainter *get_painter(void)
{
    return &painter;
}
