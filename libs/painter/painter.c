#include "IPainter.h"
#include "painter.h"
#include "../storage/fonts.h"
#include "../storage/gfx.h"
#include "../storage/post_processing.h"
#include "../storage/sprites.h"
#include "hardware/sync/spin_lock.h"
#include "../arithmetics/fpa.h"

static const IHardware *_hardware = NULL;
static const IDisplay *_display = NULL;
static uint8_t buffer[BUFFER_SIZE];
static uint8_t temp_buffer[BUFFER_SIZE];
static const uint32_t chunk_size = 15360;
static spin_lock_t *lcd_spinlock;
static uint8_t scanline_offset = 0;
static const uint8_t DEFAULT_FONT_SIZE = 8;

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

void dma_buffer_irq_handler()
{
    dma_hw->ints1 = 1u << dma_channel;
}

void init_dma()
{
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
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

void init_painter(const IDisplay *display, const IHardware *hardware)
{
    _hardware = hardware;
    _display = display;
    init_dma();
    _hardware->write(SD_CS_PIN, 1);
    _hardware->write(LCD_CS_PIN, 0);
    // lcd_spinlock=spin_lock_init(spin_lock_claim_unused(true));
}

void draw_buffer()
{
    uint32_t current_offset = 0;
    spin_lock_t *spi_spinlock = _hardware->get_spinlock();
    // uint32_t flags = spin_lock_blocking(spi_spinlock);
    // _hardware->write(SD_CS_PIN, 1);
    // _hardware->write(LCD_CS_PIN, 0);
    _hardware->write(LCD_DC_PIN, 0);
    _hardware->spi_write_byte(0x2C);
    _hardware->write(LCD_DC_PIN, 1);
    // spin_unlock(spi_spinlock, flags);
    while (current_offset < BUFFER_SIZE)
    {
        // flags = spin_lock_blocking(spi_spinlock);
        // _hardware->write(SD_CS_PIN, 1);
        // _hardware->write(LCD_CS_PIN, 0);
        dma_channel_set_read_addr(dma_channel, buffer + current_offset, true);
        dma_channel_wait_for_finish_blocking(dma_channel);
        current_offset += chunk_size;
        // spin_unlock(spi_spinlock, flags);
    }
}

void clear_buffer(uint16_t color)
{
    memset(buffer, color, sizeof(buffer));
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint32_t line_adr = (x * HEIGHT_DOUBLED) + (y * 2);
    if (line_adr >= BUFFER_SIZE || line_adr < 0)
        return;
    buffer[line_adr] = (color >> 8) & 0xff;
    buffer[line_adr + 1] = color & 0xff;
}

void draw_image(uint8_t image_index)
{
    int dma_channel_flash = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_channel_flash);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, true);
    dma_channel_configure(
        dma_channel_flash,
        &config,
        buffer,
        get_image(image_index)->image,
        BUFFER_SIZE,
        false);
    dma_channel_start(dma_channel_flash);
    dma_channel_wait_for_finish_blocking(dma_channel_flash);
    dma_channel_unclaim(dma_channel_flash);
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
    uint8_t *framebuffer = (uint8_t *)malloc(sizeof(uint8_t) * BUFFER_SIZE);
    // for (uint32_t i = 0; i < BUFFER_SIZE_HALF; i++)
    // {
    //     uint32_t index = get_effect_table_element(0, i);
    //     framebuffer[i * 2] = buffer[index * 2];
    //     framebuffer[i * 2 + 1] = buffer[index * 2 + 1];
    // }
    // memcpy(buffer, framebuffer, BUFFER_SIZE);
    // chromatic aberration
    for (uint16_t y = 0; y < DISPLAY_WIDTH; y++)
    {
        uint ydh = y * DISPLAY_HEIGHT;
        for (uint16_t x = 0; x < DISPLAY_HEIGHT; x++)
        {
            uint i = ydh + x;

            uint xr = (x > 1) ? x - 2 : x;
            uint xg = (x < DISPLAY_HEIGHT) ? x + 2 : x;

            uint ir = ydh + xr;
            uint ig = ydh + xg;

            uint16_t c_r = buffer[ir * 2] | (buffer[ir * 2 + 1] << 8);
            uint16_t c_g = buffer[ig * 2] | (buffer[ig * 2 + 1] << 8);
            uint16_t c_b = buffer[i * 2] | (buffer[i * 2 + 1] << 8);

            uint8_t r = get_r(c_r);
            uint8_t g = get_g(c_g);
            uint8_t b = get_b(c_b);
            uint16_t result = make_rgb565(r, g, b);

            framebuffer[i * 2] = result & 0xFF;
            framebuffer[i * 2 + 1] = result >> 8;
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
    uint16_t r = params[0];
    uint16_t mainColor = params[1];
    uint16_t maxOffsets = ((r << 1) + 1);
    maxOffsets *= maxOffsets;
    int offsets[maxOffsets << 1];
    uint16_t offsetsNum = 0;
    uint16_t r2 = r * r;
    for (int16_t dy = -r; dy <= r; dy++)
    {
        for (int16_t dx = -r; dx <= r; dx++)
        {
            uint16_t dist = dx * dx + dy * dy;
            if (dist <= r2)
            {
                offsets[offsetsNum] = dy * HEIGHT_DOUBLED + dx * 2;
                offsetsNum++;
                offsets[offsetsNum] = dist;
                offsetsNum++;
            }
        }
    }
    for (int i = 0; i < BUFFER_SIZE; i += 2)
    {
        uint16_t color = ((uint16_t)buffer[i] << 8) | buffer[i + 1];
        if (color != mainColor)
            continue;
        else
        {
            for (uint16_t j = 0; j < offsetsNum; j += 2)
            {
                int addr = i + offsets[j];
                if (addr < 0 || addr >= BUFFER_SIZE)
                    continue;
                uint16_t r5 = ((r2 - offsets[j + 1]) * 31 + (r2 >> 1)) / r2;
                uint16_t g6 = ((r2 - offsets[j + 1]) * 63 + (r2 >> 1)) / r2;
                r5 = (r5 * 200 + 127) / 255;
                g6 = (g6 * 200 + 127) / 255;
                uint16_t new_color = (r5 << 11) | (g6 << 5) | r5;
                if (new_color > (((uint16_t)buffer[addr] << 8) | buffer[addr + 1]))
                {
                    buffer[addr] = (new_color >> 8) & 0xff;
                    buffer[addr + 1] = new_color & 0xff;
                }
            }
        }
    }
}

void blue_only()
{
    for (uint32_t i = 0; i < BUFFER_SIZE; i += 2)
    {
        uint16_t color = ((uint16_t)buffer[i] << 8) | buffer[i + 1];
        if (color == 0)
            continue;
        uint8_t b = get_b(color);
        color = make_rgb565(3, 5, b);
        buffer[i] = (color >> 8) & 0xff;
        buffer[i + 1] = color & 0xff;
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

            uint16_t c_r = buffer[ir * 2] | (buffer[ir * 2 + 1] << 8);
            uint16_t c_g = buffer[ig * 2] | (buffer[ig * 2 + 1] << 8);
            uint16_t c_b = buffer[i * 2] | (buffer[i * 2 + 1] << 8);

            uint8_t r = get_r(c_r);
            uint8_t g = get_g(c_g);
            uint8_t b = get_b(c_b);
            uint16_t result = make_rgb565(r, g, b);

            temp_buffer[i * 2] = result & 0xFF;
            temp_buffer[i * 2 + 1] = result >> 8;
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

void draw_sprite(const Sprite *sprite, int16_t pos_y, int16_t pos_x, int32_t angle, uint8_t scale)
{
    if (angle != 0 && sprite->canRotate)
    {
        int16_t cos = fast_cos(angle);
        int16_t sin = fast_sin(angle);
        int8_t middle = sprite->size >> 1;
        for (uint16_t y = 0; y < sprite->size; y++)
        {
            int16_t new_y = y + (pos_y >> (scale - 1));
            if (new_y >= 0 && new_y < DISPLAY_HEIGHT)
            {
                for (uint16_t x = 0; x < sprite->size; x++)
                {
                    int16_t new_x = x + (pos_x >> (scale - 1));
                    int16_t xr = (((x - middle) * cos - (y - middle) * sin) >> SHIFT_FACTOR) + middle;
                    int16_t yr = (((x - middle) * sin + (y - middle) * cos) >> SHIFT_FACTOR) + middle;
                    if ((uint16_t)xr < sprite->size && (uint16_t)yr < sprite->size)
                    {
                        uint16_t pixel = sprite->pixels[yr * sprite->size + xr];
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
        for (uint16_t y = 0; y < sprite->size; y++)
        {
            int16_t new_y = y + (pos_y >> (scale - 1));
            if (new_y >= 0 && new_y < DISPLAY_HEIGHT)
            {
                uint32_t ydh = y * sprite->size;
                for (uint16_t x = 0; x < sprite->size; x++)
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

void draw_bone(Bone *bone, int *parentWorldMatrix)
{
    for (uint8_t i = 0; i < bone->childBonesNumLayer2; i++)
    {
        draw_bone(&bone->childBonesLayer2[i], bone->worldMatrix);
    }
    if (bone->sprite != NULL)
    {
        uint8_t spriteSizeHalved = bone->sprite->size >> 1;
        int16_t startX = bone->worldMatrix[2] >> SHIFT_FACTOR;
        int16_t startY = bone->worldMatrix[5] >> SHIFT_FACTOR;
        int16_t parentX = parentWorldMatrix[2] >> SHIFT_FACTOR;
        int16_t parentY = parentWorldMatrix[5] >> SHIFT_FACTOR;
        int32_t angle = 0;
        if (bone->sprite->canRotate)
        {
            angle = fast_atan2(startY - parentY, startX - parentX) + bone->baseSpriteAngle;
            angle = radian_to_index(angle);
        }
        startX += ((parentX - startX) >> 1) - spriteSizeHalved;
        startY += ((parentY - startY) >> 1) - spriteSizeHalved;
        draw_sprite(bone->sprite, startX, startY, angle, 1);
    }
    // draw_sprite(bone->sprite, x, y, 0.0f);
    for (uint8_t i = 0; i < bone->childBonesNumLayer1; i++)
    {
        draw_bone(&bone->childBonesLayer1[i], bone->worldMatrix);
    }
}

void draw_puppet(Puppet *puppet)
{
    for (uint8_t i = 0; i < puppet->bonesNum; i++)
    {
        draw_bone(&puppet->bones[i], puppet->worldMatrix);
    }
}

void print(const char *text, int16_t x, int16_t y, uint8_t scale)
{
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
        const Font *font = get_font_by_index(text[i] - 33);
        draw_sprite(font->sprite, x + offset - (((font->sprite->size - font->width) << (scale - 1)) >> 1), y + yFactor, 0, scale);
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
    if (lines > 320)
        lines = 320;
    if (mode == 0)
        memcpy(buffer, temp_buffer, lines * 480);
    else if (mode == 1)
        memcpy(temp_buffer, buffer, lines * 480);
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

    for (uint16_t x = 0; x < DISPLAY_WIDTH; x++)
    {
        uint32_t lineAddr = x * HEIGHT_DOUBLED;
        uint8_t patternAddr = (x & 3) << 2;
        for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++)
        {
            uint32_t currentLineAddr = lineAddr + (y << 1);
            uint8_t yMod4 = y & 3;
            if (pattern[patternAddr + yMod4] == 1)
            {
                buffer[currentLineAddr] = 0;
                buffer[currentLineAddr + 1] = 0;
            }
        }
    }
}

void draw_scroller(const Scroller *scroller, uint16_t x, uint16_t y, uint32_t startFrame, uint32_t currentFrame)
{
    uint16_t square[2500];
    uint8_t squareWidth = 50;
    uint8_t squareHeight = 50;
    uint8_t line = currentFrame - startFrame;
    if (startFrame > currentFrame)
        line = 0;
    else if (line > squareHeight)
        line = squareHeight;
    memcpy(square, scroller->bitmap + line * scroller->width, sizeof(square));
    uint16_t lineBuffer[scroller->width << 1];
    for (uint8_t i = 0; i < squareHeight; i++)
    {
        for (uint8_t j = 0; j < squareWidth; j++)
        {
            lineBuffer[j * 2] = __builtin_bswap16(square[i * squareWidth + j]);
            lineBuffer[j * 2 + 1] = __builtin_bswap16(square[i * squareWidth + j]);
        }
        uint32_t lineAddr1 = (y + i * 2) * HEIGHT_DOUBLED + x * 2;
        uint32_t lineAddr2 = (y + i * 2 + 1) * HEIGHT_DOUBLED + x * 2;
        memcpy(buffer + lineAddr1, lineBuffer, sizeof(lineBuffer));
        memcpy(buffer + lineAddr2, lineBuffer, sizeof(lineBuffer));
    }
}

void fade(uint8_t mode, uint32_t startFrame, uint32_t currentFrame, uint16_t y, uint16_t x, uint16_t width, uint16_t height)
{
    if (currentFrame < startFrame)
        return;
    uint8_t patternIndex = currentFrame - startFrame;
    if(patternIndex>8)
        return;
    if ((patternIndex == 8 && mode == 0))
        return;
    uint8_t pattern[16];
    uint16_t square[width * height];

    if (mode == 0) // fade out
    {
        memcpy(pattern, fadeOutPatterns[patternIndex], 16);
    }
    else // fade in
    {
        memcpy(pattern, fadeInPatterns[patternIndex], 16);
    }

    uint8_t lineBuffer[width << 1];

    for (uint8_t i = 0; i < height; i += 1)
    {
        uint32_t lineAddr = (i + x) * HEIGHT_DOUBLED;
        uint8_t patternAddr = (i & 3) << 2;
        for (uint8_t j = 0; j < width; j += 1)
        {
            uint32_t currentLineAddr = lineAddr + ((y + j) << 1);
            uint8_t yMod4 = j & 3;
            if (pattern[patternAddr + yMod4] == 1)
            {
                lineBuffer[(j<<1)] = buffer[currentLineAddr];
                lineBuffer[(j<<1) + 1] = buffer[currentLineAddr + 1];
            }
            else
            {
                lineBuffer[(j<<1)] = temp_buffer[currentLineAddr];
                lineBuffer[(j<<1) + 1] = temp_buffer[currentLineAddr + 1];
            }
        }
        lineAddr += (y << 1);
        memcpy(buffer + lineAddr, lineBuffer, sizeof(lineBuffer));
    }
}

static IPainter painter = {
    .init_painter = init_painter,
    .draw_buffer = draw_buffer,
    .clear_buffer = clear_buffer,
    .draw_pixel = draw_pixel,
    .draw_image = draw_image,
    .apply_post_process_effect = apply_post_process_effect,
    .draw_sprite = draw_sprite,
    .draw_puppet = draw_puppet,
    .print = print,
    .draw_gradient = draw_gradient,
    .override_buffer = override_buffer,
    .fade_fullscreen = fade_fullscreen,
    .draw_scroller = draw_scroller,
    .fade = fade,
};

const IPainter *get_painter(void)
{
    return &painter;
}