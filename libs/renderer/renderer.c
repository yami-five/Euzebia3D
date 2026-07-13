#include "IRenderer.h"
#include "renderer.h"
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
#include "hardware/interp.h"
#endif
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

static const IHardware *_hardware = NULL;
static const IPainter *_painter = NULL;

static const uint8_t FOCAL_LENGTH = 90;
static const uint32_t FIRE_FLOOR_ADR = 76480;
static const uint32_t FIXED_FOCAL_LENGTH = 90 << SHIFT_FACTOR;
static const uint32_t TRIANGLE_CENTER_DIVIDER = 3 << SHIFT_FACTOR;
static const uint16_t BASE_WIDTH = 320;
static const uint16_t BASE_HEIGHT = 240;

static uint8_t render_scale = 1; // 2 => 160x120 render; 1 => 320x240 render
static uint8_t output_scale = 1;
static uint16_t render_width = 160;
static uint16_t render_height = 120;
static uint16_t render_width_half = 80;
static uint16_t render_height_half = 60;

#define MAX_TRIANGLES_IN_SCENE 1500
#define SPAN_BUFFER_MAX 320
#define SHADING_ENABLED 1
#define LIGHT_LERP_SHIFT 8
#define UV_LERP_SHIFT 8
#define UV_PERSPECTIVE_SHIFT 6
#define MAX_SHADING_SPAN_LEN 320
#define SCREEN_COORD_GUARD 1024
#define NEAR_CLIP_Z 1
#define MAX_MODEL_TRANSFORMATIONS 8
#define TEXTURE_TRANSPARENT_COLOR 0xf81fu
// Render can be downscaled: render_scale=2 -> 160x120 rendered, scaled to LCD in painter.

static TriangleInScene scene[MAX_TRIANGLES_IN_SCENE];
static uint16_t sceneCounter = 0;
static uint16_t span_buffer[SPAN_BUFFER_MAX];
static uint16_t span_scaled_buffer[SPAN_BUFFER_MAX];
static uint16_t span_length = 0;

static inline int32_t clamp_i64_to_i32(int64_t value)
{
    if (value > INT32_MAX)
        return INT32_MAX;
    if (value < INT32_MIN)
        return INT32_MIN;
    return (int32_t)value;
}

static inline int32_t shift_left_clamped_i32(int32_t value, uint8_t shift)
{
    return clamp_i64_to_i32((int64_t)value << shift);
}

static inline int32_t shifted_delta_i32(int32_t a, int32_t b, int32_t shift, int32_t divisor)
{
    if (divisor == 0)
        return 0;

    int64_t value = ((int64_t)b - (int64_t)a) << shift;
    return clamp_i64_to_i32(value / divisor);
}

static inline int32_t make_perspective_uv(int32_t uv, int32_t invZ)
{
    int64_t value = (int64_t)uv * (int64_t)invZ;
    value >>= (SHIFT_FACTOR - UV_PERSPECTIVE_SHIFT);
    return clamp_i64_to_i32(value);
}

static inline int32_t restore_perspective_uv(int32_t uvOverZ, int32_t z)
{
    int64_t value = (int64_t)uvOverZ * (int64_t)z;
    value >>= (SHIFT_FACTOR + UV_PERSPECTIVE_SHIFT);
    return clamp_i64_to_i32(value);
}

static inline int32_t texture_dimension_shift(int32_t size)
{
    if (size <= 0 || (size & (size - 1)) != 0)
        return -1;

    int32_t shift = 0;
    while (size > 1)
    {
        size >>= 1;
        shift++;
    }
    return shift;
}

#if EUZEBIA3D_DEBUG_STAGE_ENABLED
volatile uint32_t renderer_debug_stage = 0;
#define RENDERER_SET_DEBUG_STAGE(stage) \
    do                                  \
    {                                   \
        renderer_debug_stage = (stage); \
    } while (0)
#else
#define RENDERER_SET_DEBUG_STAGE(stage) ((void)0)
#endif
volatile uint32_t renderer_debug_vertex_index = 0;
volatile uint32_t renderer_debug_face_index = 0;
volatile uint32_t renderer_debug_scene_counter = 0;
volatile uint32_t renderer_debug_transform_count = 0;
volatile uintptr_t renderer_debug_pointer = 0;

// Build-time knobs injected from CMake (with safe fallbacks).
#ifndef EUZEBIA3D_TEXTURE_CACHE_ENABLED
#define EUZEBIA3D_TEXTURE_CACHE_ENABLED 1
#endif
#ifndef EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH
#define EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH 1
#endif
#ifndef EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM
#define EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM 1
#endif
#ifndef EUZEBIA3D_TEXTURE_CACHE_SIZE
#define EUZEBIA3D_TEXTURE_CACHE_SIZE 64
#endif
#ifndef EUZEBIA3D_TEXTURE_CACHE_SLOTS
#define EUZEBIA3D_TEXTURE_CACHE_SLOTS 2
#endif

#if (EUZEBIA3D_TEXTURE_CACHE_SIZE < 1)
#undef EUZEBIA3D_TEXTURE_CACHE_SIZE
#define EUZEBIA3D_TEXTURE_CACHE_SIZE 1
#endif
#if (EUZEBIA3D_TEXTURE_CACHE_SLOTS < 1)
#undef EUZEBIA3D_TEXTURE_CACHE_SLOTS
#define EUZEBIA3D_TEXTURE_CACHE_SLOTS 1
#endif

#define TEXTURE_CACHE_SIZE EUZEBIA3D_TEXTURE_CACHE_SIZE
#define TEXTURE_CACHE_PIXELS (TEXTURE_CACHE_SIZE * TEXTURE_CACHE_SIZE)
#define TEXTURE_CACHE_SLOTS EUZEBIA3D_TEXTURE_CACHE_SLOTS

#if EUZEBIA3D_TEXTURE_CACHE_ENABLED
typedef struct
{
    const uint16_t *source;
    int32_t width;
    int32_t height;
    uint8_t valid;
    uint16_t pixels[TEXTURE_CACHE_PIXELS];
} TextureCacheSlot;

typedef enum
{
    TEXTURE_SOURCE_UNKNOWN = 0,
    TEXTURE_SOURCE_FLASH = 1,
    TEXTURE_SOURCE_PSRAM = 2
} TextureSource;

static TextureCacheSlot textureCache[TEXTURE_CACHE_SLOTS] = {0};
static uint8_t textureCacheNextSlot = 0;

static inline TextureSource detect_texture_source(const uint16_t *texture)
{
    (void)texture;
#if defined(EUZEBIA3D_PLATFORM_PICO)
    uintptr_t address = (uintptr_t)texture;
    uint32_t region = (uint32_t)(address & 0xff000000u);
    if (region == 0x10000000u)
        return TEXTURE_SOURCE_FLASH;
    if (region == 0x11000000u)
        return TEXTURE_SOURCE_PSRAM;
#endif
    return TEXTURE_SOURCE_UNKNOWN;
}

static inline uint8_t texture_cache_source_enabled(TextureSource source)
{
    if (source == TEXTURE_SOURCE_FLASH)
        return (uint8_t)(EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH ? 1 : 0);
    if (source == TEXTURE_SOURCE_PSRAM)
        return (uint8_t)(EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM ? 1 : 0);
#if (EUZEBIA3D_TEXTURE_CACHE_FROM_FLASH || EUZEBIA3D_TEXTURE_CACHE_FROM_PSRAM)
    return 1;
#else
    return 0;
#endif
}
#endif

static const uint16_t *activeTextureData = NULL;
static int32_t activeTextureWidth = 0;
static int32_t activeTextureHeight = 0;
static int32_t activeTextureWidthShift = -1;
static int32_t activeTextureHeightShift = -1;

static inline void set_active_texture_data(const uint16_t *texture, int32_t width, int32_t height)
{
    activeTextureData = texture;
    activeTextureWidth = width;
    activeTextureHeight = height;
    activeTextureWidthShift = texture_dimension_shift(width);
    activeTextureHeightShift = texture_dimension_shift(height);
}

static inline void set_active_texture_direct(Material *mat)
{
    if (mat == NULL)
    {
        set_active_texture_data(NULL, 0, 0);
        return;
    }
    set_active_texture_data(mat->texture, mat->textureWidth, mat->textureHeight);
}

static void prepare_texture_cache(Material *mat)
{
    set_active_texture_direct(mat);

#if EUZEBIA3D_TEXTURE_CACHE_ENABLED
    if (mat == NULL || mat->texture == NULL || mat->textureWidth <= 0 || mat->textureHeight <= 0)
        return;

    if (mat->textureWidth > TEXTURE_CACHE_SIZE || mat->textureHeight > TEXTURE_CACHE_SIZE)
        return;

    size_t texelCount = (size_t)mat->textureWidth * (size_t)mat->textureHeight;
    if (texelCount > TEXTURE_CACHE_PIXELS)
        return;

    if (!texture_cache_source_enabled(detect_texture_source(mat->texture)))
        return;

    for (uint8_t i = 0; i < TEXTURE_CACHE_SLOTS; i++)
    {
        if (textureCache[i].valid &&
            textureCache[i].source == mat->texture &&
            textureCache[i].width == mat->textureWidth &&
            textureCache[i].height == mat->textureHeight)
        {
            set_active_texture_data(textureCache[i].pixels, textureCache[i].width, textureCache[i].height);
            return;
        }
    }

    TextureCacheSlot *slot = &textureCache[textureCacheNextSlot];
    textureCacheNextSlot++;
    if (textureCacheNextSlot >= TEXTURE_CACHE_SLOTS)
        textureCacheNextSlot = 0;

    memcpy(slot->pixels, mat->texture, texelCount * sizeof(uint16_t));
    slot->source = mat->texture;
    slot->width = mat->textureWidth;
    slot->height = mat->textureHeight;
    slot->valid = 1;

    set_active_texture_data(slot->pixels, slot->width, slot->height);
#endif
}

/* Pico path uses hardware interpolators to speed up span setup. */
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
static void init_span_interpolators(void)
{
    interp_config uv_cfg = interp_default_config();
    interp_config_set_signed(&uv_cfg, true);
    interp_set_config(interp0, 0, &uv_cfg);
    interp_set_config(interp0, 1, &uv_cfg);

    interp_config w_cfg = interp_default_config();
    interp_config_set_signed(&w_cfg, true);
    interp_set_config(interp1, 0, &w_cfg);

    interp_config l_cfg = interp_default_config();
    interp_config_set_signed(&l_cfg, true);
    interp_set_config(interp1, 1, &l_cfg);

    // Keep lane BASE registers neutral - span code uses raw lane values only.
    interp_set_base(interp0, 0, 0);
    interp_set_base(interp0, 1, 0);
    interp_set_base(interp0, 2, 0);
    interp_set_base(interp1, 0, 0);
    interp_set_base(interp1, 1, 0);
    interp_set_base(interp1, 2, 0);
}
#endif

// Scratch buffers reused between frames/models to avoid frequent heap churn.
static int32_t *modelScratchVerticesModified = NULL;
static int32_t *modelScratchVerticesClip = NULL;
static int32_t *modelScratchNormalsModified = NULL;
static uint16_t modelScratchVertexCapacity = 0;
static uint16_t modelScratchNormalCapacity = 0;

static uint8_t ensure_model_scratch_capacity(uint16_t verticesCounter, uint16_t vnCounter)
{
    if (verticesCounter > modelScratchVertexCapacity)
    {
        size_t vertexTriplets = (size_t)verticesCounter * 3u;
        size_t vertexQuads = (size_t)verticesCounter * 4u;

        int32_t *newVerticesModified = (int32_t *)realloc(modelScratchVerticesModified, sizeof(int32_t) * vertexTriplets);
        if (newVerticesModified == NULL)
            return 0;
        modelScratchVerticesModified = newVerticesModified;

        int32_t *newVerticesClip = (int32_t *)realloc(modelScratchVerticesClip, sizeof(int32_t) * vertexQuads);
        if (newVerticesClip == NULL)
            return 0;
        modelScratchVerticesClip = newVerticesClip;

        modelScratchVertexCapacity = verticesCounter;
    }

    if (vnCounter > modelScratchNormalCapacity)
    {
        size_t normalTriplets = (size_t)vnCounter * 3u;
        int32_t *newNormalsModified = (int32_t *)realloc(modelScratchNormalsModified, sizeof(int32_t) * normalTriplets);
        if (newNormalsModified == NULL)
            return 0;
        modelScratchNormalsModified = newNormalsModified;
        modelScratchNormalCapacity = vnCounter;
    }

    return 1;
}

// Normalize vector but bail out on zero-length to avoid NaNs.
static inline uint8_t norm_vector_safe(Vector3 *vec)
{
    int32_t len = len_vector(vec);
    if (len == 0)
        return 0;
    vec->x = fixed_div(vec->x, len);
    vec->y = fixed_div(vec->y, len);
    vec->z = fixed_div(vec->z, len);
    return 1;
}

static inline uint8_t read_normal_vector(Vector3 *out, const int32_t *normals, uint16_t normalsCounter, uint16_t normalIndex)
{
    if (normalIndex >= normalsCounter)
        return 0;

    size_t base = (size_t)normalIndex * 3u;
    out->x = normals[base];
    out->y = normals[base + 1u];
    out->z = normals[base + 2u];
    return 1;
}

static inline void write_vector_triplet(int32_t *vectors, uint16_t vectorIndex, const Vector3 *value)
{
    size_t base = (size_t)vectorIndex * 3u;
    vectors[base] = value->x;
    vectors[base + 1u] = value->y;
    vectors[base + 2u] = value->z;
}

static inline void zero_vector(Vector3 *value)
{
    value->x = 0;
    value->y = 0;
    value->z = 0;
}

static inline void make_light_direction(Vector3 *out, const Light *light, const int32_t *vertices, size_t vertexBase)
{
    if (light->lightType == DIRECTIONAL_LIGHT)
    {
        out->x = light->position.x;
        out->y = light->position.y;
        out->z = light->position.z;
    }
    else
    {
        out->x = light->position.x - vertices[vertexBase];
        out->y = light->position.y - vertices[vertexBase + 1u];
        out->z = light->position.z - vertices[vertexBase + 2u];
    }

    if (!norm_vector_safe(out))
        zero_vector(out);
}

static inline int32_t clamp_light_distance(int32_t lightDistance)
{
    if (lightDistance < 0)
        return 0;
    if (lightDistance > SCALE_FACTOR)
        return SCALE_FACTOR;
    return lightDistance;
}

static void configure_render_dimensions(void)
{
    if (render_scale == 0)
        render_scale = 2;
    output_scale = render_scale;
    render_width = BASE_WIDTH / render_scale;
    render_height = BASE_HEIGHT / render_scale;
    render_width_half = render_width >> 1;
    render_height_half = render_height >> 1;
}

void renderer_set_scale(uint8_t scale)
{
    if (scale == 0)
        return;
    render_scale = scale;
    configure_render_dimensions();
}

void init_renderer(const IHardware *hardware, const IPainter *painter)
{
    _hardware = hardware;
    _painter = painter;
    configure_render_dimensions();
#if !defined(EUZEBIA3D_PLATFORM_WINDOWS)
    init_span_interpolators();
#endif
    // init_sin_cos();
}

void triangle_center(Triangle3D *triangle, int32_t *center)
{
    center[0] = fixed_div((triangle->a.x + triangle->b.x + triangle->c.x), TRIANGLE_CENTER_DIVIDER);
    center[1] = fixed_div((triangle->a.y + triangle->b.y + triangle->c.y), TRIANGLE_CENTER_DIVIDER);
    center[2] = fixed_div((triangle->a.z + triangle->b.z + triangle->c.z), TRIANGLE_CENTER_DIVIDER);
}

void rotate(int32_t *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    RENDERER_SET_DEBUG_STAGE(600);
    renderer_debug_scene_counter = verticesCounter;
    renderer_debug_pointer = (uintptr_t)vector;

    if (vertices == NULL || vector == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(601);
        return;
    }

    int32_t qt_rad = fixed_mul(vector->w, PI2);
    int32_t c = fast_cos(qt_rad >> 1);
    int32_t s = fast_sin(qt_rad >> 1);
    RENDERER_SET_DEBUG_STAGE(610);

    Vector3 qVec = {
        .x = vector->x,
        .y = vector->y,
        .z = vector->z};
    Quaternion q = {
        .w = c,
        .vec = &qVec};
    if (!norm_vector_safe(q.vec))
    {
        RENDERER_SET_DEBUG_STAGE(611);
        return;
    }

    mul_vec_scalar(q.vec, s);
    RENDERER_SET_DEBUG_STAGE(620);

    Vector3 qVecInv = {
        .x = -q.vec->x,
        .y = -q.vec->y,
        .z = -q.vec->z};
    Quaternion qInv = {
        .w = c,
        .vec = &qVecInv};
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        RENDERER_SET_DEBUG_STAGE(630);
        renderer_debug_vertex_index = i;
        Vector3 vec_vertex =
            {
                .x = vertices[i * 3],
                .y = vertices[i * 3 + 1],
                .z = vertices[i * 3 + 2]};
        Quaternion q_vertex = {
            .w = 0,
            .vec = &vec_vertex};
        Vector3 resultVec1;
        Quaternion result = {
            .w = 0,
            .vec = &resultVec1};
        mul_quaternion(&result, &q, &q_vertex);
        Vector3 resultVec2;
        Quaternion result2 = {
            .w = 0,
            .vec = &resultVec2};
        mul_quaternion(&result2, &result, &qInv);
        vertices[i * 3] = result2.vec->x;
        vertices[i * 3 + 1] = result2.vec->y;
        vertices[i * 3 + 2] = result2.vec->z;
    }

    RENDERER_SET_DEBUG_STAGE(690);
}

void translate(int32_t *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    RENDERER_SET_DEBUG_STAGE(700);
    renderer_debug_scene_counter = verticesCounter;
    renderer_debug_pointer = (uintptr_t)vector;

    if (vertices == NULL || vector == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(701);
        return;
    }

    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        renderer_debug_vertex_index = i;
        vertices[i * 3] += vector->x;
        vertices[i * 3 + 1] += vector->y;
        vertices[i * 3 + 2] += vector->z;
    }

    RENDERER_SET_DEBUG_STAGE(790);
}

void scale(int32_t *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    RENDERER_SET_DEBUG_STAGE(800);
    renderer_debug_scene_counter = verticesCounter;
    renderer_debug_pointer = (uintptr_t)vector;

    if (vertices == NULL || vector == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(801);
        return;
    }

    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        renderer_debug_vertex_index = i;
        vertices[i * 3] = fixed_mul(vertices[i * 3], vector->x);
        vertices[i * 3 + 1] = fixed_mul(vertices[i * 3 + 1], vector->y);
        vertices[i * 3 + 2] = fixed_mul(vertices[i * 3 + 2], vector->z);
    }

    RENDERER_SET_DEBUG_STAGE(890);
}

void transform(int32_t *vertices, uint16_t verticesCounter, TransformInfo *transformInfo)
{
    RENDERER_SET_DEBUG_STAGE(900);
    renderer_debug_scene_counter = verticesCounter;
    renderer_debug_pointer = (uintptr_t)transformInfo;

    if (vertices == NULL || transformInfo == NULL || transformInfo->transformVector == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(901);
        return;
    }

    RENDERER_SET_DEBUG_STAGE(910 + transformInfo->transformType);
    if (transformInfo->transformType == MODEL_TRANSFORM_ROTATE)
        rotate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == MODEL_TRANSFORM_TRANSLATE)
        translate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == MODEL_TRANSFORM_SCALE)
        scale(vertices, verticesCounter, transformInfo->transformVector);

    RENDERER_SET_DEBUG_STAGE(990);
}

void inf(float *x, float *y, float qt)
{
    float qt_rad = qt * PI2;
    *x += 2.0f * (fast_cos(qt_rad));
    *y += 2.0f * fast_cos(qt_rad) * fast_sin(qt_rad);
}

int32_t check_if_triangle_visible(Triangle2D *triangle)
{
    int32_t e1x = triangle->b.x - triangle->a.x;
    int32_t e1y = triangle->b.y - triangle->a.y;
    int32_t e2x = triangle->c.x - triangle->a.x;
    int32_t e2y = triangle->c.y - triangle->a.y;

    return ((int64_t)e1x * e2y - (int64_t)e1y * e2x) >= 0;
}

static uint8_t triangle_outside_render_area(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cx, int32_t cy)
{
    if (ax < 0 && bx < 0 && cx < 0)
        return 1;
    if (ay < 0 && by < 0 && cy < 0)
        return 1;
    if (ax >= render_width && bx >= render_width && cx >= render_width)
        return 1;
    if (ay >= render_height && by >= render_height && cy >= render_height)
        return 1;
    return 0;
}

static uint8_t triangle_outside_raster_guard(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cx, int32_t cy)
{
    int32_t minX = -SCREEN_COORD_GUARD;
    int32_t minY = -SCREEN_COORD_GUARD;
    int32_t maxX = (int32_t)render_width - 1 + SCREEN_COORD_GUARD;
    int32_t maxY = (int32_t)render_height - 1 + SCREEN_COORD_GUARD;

    if (ax < minX || ax > maxX || ay < minY || ay > maxY)
        return 1;
    if (bx < minX || bx > maxX || by < minY || by > maxY)
        return 1;
    if (cx < minX || cx > maxX || cy < minY || cy > maxY)
        return 1;
    return 0;
}

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t w;
    int32_t uvx;
    int32_t uvy;
    int32_t light;
} ClipVertex;

static inline uint8_t is_inside_near_plane(const ClipVertex *v)
{
    return v->z >= NEAR_CLIP_Z;
}

static inline int32_t interpolate_fixed(int32_t a, int32_t b, int32_t t)
{
    return a + (int32_t)fixed_mul((b - a), t);
}

static ClipVertex intersect_near_plane(const ClipVertex *a, const ClipVertex *b)
{
    ClipVertex out = *a;
    int32_t dz = b->z - a->z;
    if (dz == 0)
    {
        out.z = NEAR_CLIP_Z;
        return out;
    }

    int32_t t = fixed_div(NEAR_CLIP_Z - a->z, dz);
    if (t < 0)
        t = 0;
    if (t > SCALE_FACTOR)
        t = SCALE_FACTOR;

    out.x = interpolate_fixed(a->x, b->x, t);
    out.y = interpolate_fixed(a->y, b->y, t);
    out.z = NEAR_CLIP_Z;
    out.w = interpolate_fixed(a->w, b->w, t);
    out.uvx = interpolate_fixed(a->uvx, b->uvx, t);
    out.uvy = interpolate_fixed(a->uvy, b->uvy, t);
    out.light = interpolate_fixed(a->light, b->light, t);
    return out;
}

static uint8_t clip_triangle_against_near_plane(const ClipVertex in[3], ClipVertex out[4])
{
    uint8_t outCount = 0;
    ClipVertex prev = in[2];
    uint8_t prevInside = is_inside_near_plane(&prev);

    for (uint8_t i = 0; i < 3; i++)
    {
        ClipVertex curr = in[i];
        uint8_t currInside = is_inside_near_plane(&curr);

        if (currInside)
        {
            if (!prevInside && outCount < 4)
                out[outCount++] = intersect_near_plane(&prev, &curr);
            if (outCount < 4)
                out[outCount++] = curr;
        }
        else if (prevInside)
        {
            if (outCount < 4)
                out[outCount++] = intersect_near_plane(&prev, &curr);
        }

        prev = curr;
        prevInside = currInside;
    }
    return outCount;
}

void shading(uint16_t *color, Light *light, int32_t lightDistance)
{
    if (*color == TEXTURE_TRANSPARENT_COLOR)
        return;

    // Clamp minimum light to keep pixels from going fully dark on edges
    const int32_t AMBIENT_MIN = SCALE_FACTOR >> 5;
    if (lightDistance < AMBIENT_MIN)
        lightDistance = AMBIENT_MIN;
    if (lightDistance > SCALE_FACTOR)
        lightDistance = SCALE_FACTOR;

    uint8_t rMesh = (*color >> 11) & 0x1f;
    uint8_t gMesh = (*color >> 5) & 0x3f;
    uint8_t bMesh = *color & 0x1f;

    uint8_t rLight = (light->color >> 11) & 0x1f;
    uint8_t gLight = (light->color >> 5) & 0x3f;
    uint8_t bLight = light->color & 0x1f;

    uint32_t fixedR = (rMesh * rLight) << SHIFT_FACTOR;
    uint32_t fixedG = (gMesh * gLight) << SHIFT_FACTOR;
    uint32_t fixedB = (bMesh * bLight) << SHIFT_FACTOR;

    fixedR = fixed_mul(fixedR, 33);
    fixedG = fixed_mul(fixedG, 16);
    fixedB = fixed_mul(fixedB, 33);

    int32_t intensity = light->intensity;
    if (intensity < 0)
        intensity = 0;
    const int32_t INTENSITY_MAX = SCALE_FACTOR * 6;
    if (intensity > INTENSITY_MAX)
        intensity = INTENSITY_MAX;

    // LightColor * MaterialColor scaled by light factor
    int32_t lightFactor = fixed_mul(lightDistance, intensity);
    if (lightFactor < 0)
        lightFactor = 0;
    const int32_t MAX_LIGHT_FACTOR = SCALE_FACTOR * 4;
    if (lightFactor > MAX_LIGHT_FACTOR)
        lightFactor = MAX_LIGHT_FACTOR;

    uint32_t rTmp = (uint32_t)(fixed_mul(fixedR, lightFactor) >> SHIFT_FACTOR);
    uint32_t gTmp = (uint32_t)(fixed_mul(fixedG, lightFactor) >> SHIFT_FACTOR);
    uint32_t bTmp = (uint32_t)(fixed_mul(fixedB, lightFactor) >> SHIFT_FACTOR);

    if (rTmp > 31)
        rTmp = 31;
    if (gTmp > 63)
        gTmp = 63;
    if (bTmp > 31)
        bTmp = 31;

    uint8_t r = (uint8_t)rTmp;
    uint8_t g = (uint8_t)gTmp;
    uint8_t b = (uint8_t)bTmp;

    *color = (r << 11) | (g << 5) | b;
}

static inline void add_opaque_texel(uint16_t color, uint32_t *r, uint32_t *g, uint32_t *b, uint32_t *count)
{
    if (color == TEXTURE_TRANSPARENT_COLOR)
        return;

    *r += (color >> 11) & 0x1f;
    *g += (color >> 5) & 0x3f;
    *b += color & 0x1f;
    *count += 1u;
}

static inline uint16_t sample_texture_2x2(const uint16_t *texture, int32_t row0, int32_t row1, int32_t x0, int32_t x1, bool transparent)
{
    uint16_t c00 = texture[row0 + x0];
    uint16_t c10 = texture[row0 + x1];
    uint16_t c01 = texture[row1 + x0];
    uint16_t c11 = texture[row1 + x1];

    if (transparent)
    {
        if (c00 == TEXTURE_TRANSPARENT_COLOR)
            return TEXTURE_TRANSPARENT_COLOR;

        if (c10 == TEXTURE_TRANSPARENT_COLOR || c01 == TEXTURE_TRANSPARENT_COLOR || c11 == TEXTURE_TRANSPARENT_COLOR)
        {
            uint32_t r = 0;
            uint32_t g = 0;
            uint32_t b = 0;
            uint32_t count = 0;
            add_opaque_texel(c00, &r, &g, &b, &count);
            add_opaque_texel(c10, &r, &g, &b, &count);
            add_opaque_texel(c01, &r, &g, &b, &count);
            add_opaque_texel(c11, &r, &g, &b, &count);
            if (count == 0)
                return TEXTURE_TRANSPARENT_COLOR;
            return ((r / count) << 11) | ((g / count) << 5) | (b / count);
        }
    }

    uint32_t r00 = (c00 >> 11) & 0x1f;
    uint32_t g00 = (c00 >> 5) & 0x3f;
    uint32_t b00 = c00 & 0x1f;
    uint32_t r10 = (c10 >> 11) & 0x1f;
    uint32_t g10 = (c10 >> 5) & 0x3f;
    uint32_t b10 = c10 & 0x1f;
    uint32_t r01 = (c01 >> 11) & 0x1f;
    uint32_t g01 = (c01 >> 5) & 0x3f;
    uint32_t b01 = c01 & 0x1f;
    uint32_t r11 = (c11 >> 11) & 0x1f;
    uint32_t g11 = (c11 >> 5) & 0x3f;
    uint32_t b11 = c11 & 0x1f;

    uint32_t rTop = (r00 + r10) >> 1;
    uint32_t gTop = (g00 + g10) >> 1;
    uint32_t bTop = (b00 + b10) >> 1;
    uint32_t rBot = (r01 + r11) >> 1;
    uint32_t gBot = (g01 + g11) >> 1;
    uint32_t bBot = (b01 + b11) >> 1;

    uint32_t r = (rTop + rBot) >> 1;
    uint32_t g = (gTop + gBot) >> 1;
    uint32_t b = (bTop + bBot) >> 1;

    return (r << 11) | (g << 5) | b;
}

static inline void clamp_uv_fixed(int32_t *uv_x, int32_t *uv_y)
{
    if (*uv_x < 0)
        *uv_x = 0;
    if (*uv_y < 0)
        *uv_y = 0;
    if (*uv_x > SCALE_FACTOR)
        *uv_x = SCALE_FACTOR;
    if (*uv_y > SCALE_FACTOR)
        *uv_y = SCALE_FACTOR;
}

static inline void clamp_texel_coords(int32_t textureWidth, int32_t textureHeight, int32_t *tex_x, int32_t *tex_y)
{
    int32_t minX = textureWidth > 2 ? 1 : 0;
    int32_t minY = textureHeight > 2 ? 1 : 0;
    int32_t maxX = textureWidth > 2 ? textureWidth - 2 : textureWidth - 1;
    int32_t maxY = textureHeight > 2 ? textureHeight - 2 : textureHeight - 1;
    if (*tex_x < minX)
        *tex_x = minX;
    if (*tex_y < minY)
        *tex_y = minY;
    if (*tex_x > maxX)
        *tex_x = maxX;
    if (*tex_y > maxY)
        *tex_y = maxY;
}

static inline uint16_t texturing_power_of_two(const uint16_t *texture, int32_t textureWidth, int32_t textureHeight, int32_t textureWidthShift, int32_t textureHeightShift, int32_t U, int32_t V, int32_t Z, bool transparent)
{
    if (texture == NULL || textureWidth <= 0 || textureHeight <= 0)
        return TEXTURE_TRANSPARENT_COLOR;

    int32_t uv_x = restore_perspective_uv(U, Z);
    int32_t uv_y = restore_perspective_uv(V, Z);
    clamp_uv_fixed(&uv_x, &uv_y);

    int32_t tex_x = uv_x >> (SHIFT_FACTOR - textureWidthShift);
    int32_t tex_y = uv_y >> (SHIFT_FACTOR - textureHeightShift);
    clamp_texel_coords(textureWidth, textureHeight, &tex_x, &tex_y);

    int32_t x0 = tex_x;
    int32_t y0 = tex_y;
    int32_t x1 = x0 + 1 < textureWidth ? x0 + 1 : x0;
    int32_t y1 = y0 + 1 < textureHeight ? y0 + 1 : y0;
    int32_t row0 = y0 << textureWidthShift;
    int32_t row1 = y1 << textureWidthShift;

    return sample_texture_2x2(texture, row0, row1, x0, x1, transparent);
}

static inline uint16_t texturing_generic(const uint16_t *texture, int32_t textureWidth, int32_t textureHeight, int32_t U, int32_t V, int32_t Z, bool transparent)
{
    if (texture == NULL || textureWidth <= 0 || textureHeight <= 0)
        return TEXTURE_TRANSPARENT_COLOR;

    int32_t uv_x = restore_perspective_uv(U, Z);
    int32_t uv_y = restore_perspective_uv(V, Z);
    clamp_uv_fixed(&uv_x, &uv_y);

    int32_t tex_x = (uv_x * textureWidth) >> SHIFT_FACTOR;
    int32_t tex_y = (uv_y * textureHeight) >> SHIFT_FACTOR;
    clamp_texel_coords(textureWidth, textureHeight, &tex_x, &tex_y);

    int32_t x0 = tex_x;
    int32_t y0 = tex_y;
    int32_t x1 = x0 + 1 < textureWidth ? x0 + 1 : x0;
    int32_t y1 = y0 + 1 < textureHeight ? y0 + 1 : y0;
    int32_t row0 = y0 * textureWidth;
    int32_t row1 = y1 * textureWidth;

    return sample_texture_2x2(texture, row0, row1, x0, x1, transparent);
}

static void draw_masked_span(uint16_t x, uint16_t y, const uint16_t *span, uint16_t length)
{
    uint16_t i = 0;
    while (i < length)
    {
        while (i < length && span[i] == TEXTURE_TRANSPARENT_COLOR)
            i++;

        uint16_t start = i;
        while (i < length && span[i] != TEXTURE_TRANSPARENT_COLOR)
            i++;

        if (i > start)
            _painter->draw_span(x + start, y, span + start, i - start);
    }
}

static inline void fill_span(uint16_t *dst, uint16_t length, uint16_t color)
{
    for (uint16_t i = 0; i < length; i++)
        dst[i] = color;
}

static void texture_span(uint16_t *dst, uint16_t length, Material *mat, int32_t U, int32_t dUdx, int32_t V, int32_t dVdx, int32_t Z, int32_t dZdx)
{
    const uint16_t *texture = activeTextureData;
    int32_t textureWidth = activeTextureWidth;
    int32_t textureHeight = activeTextureHeight;
    int32_t textureWidthShift = activeTextureWidthShift;
    int32_t textureHeightShift = activeTextureHeightShift;
    bool transparent = mat->transparent;
    uint8_t usePowerOfTwo = (textureWidthShift >= 0 &&
                             textureHeightShift >= 0 &&
                             textureWidthShift <= SHIFT_FACTOR &&
                             textureHeightShift <= SHIFT_FACTOR);

#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
    int32_t Uacc = U;
    int32_t Vacc = V;
    int32_t Zacc = Z;

    if (usePowerOfTwo)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Ucur = Uacc >> UV_LERP_SHIFT;
            int32_t Vcur = Vacc >> UV_LERP_SHIFT;
            int32_t Zcur = Zacc;
            dst[i] = texturing_power_of_two(texture, textureWidth, textureHeight, textureWidthShift, textureHeightShift, Ucur, Vcur, Zcur, transparent);
            Uacc += dUdx;
            Vacc += dVdx;
            Zacc += dZdx;
        }
    }
    else
    {
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Ucur = Uacc >> UV_LERP_SHIFT;
            int32_t Vcur = Vacc >> UV_LERP_SHIFT;
            int32_t Zcur = Zacc;
            dst[i] = texturing_generic(texture, textureWidth, textureHeight, Ucur, Vcur, Zcur, transparent);
            Uacc += dUdx;
            Vacc += dVdx;
            Zacc += dZdx;
        }
    }
#else
    interp_set_accumulator(interp0, 0, (uint32_t)U);
    interp_set_accumulator(interp0, 1, (uint32_t)V);
    interp_set_accumulator(interp1, 0, (uint32_t)Z);

    if (usePowerOfTwo)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Ucur = ((int32_t)interp_get_accumulator(interp0, 0)) >> UV_LERP_SHIFT;
            int32_t Vcur = ((int32_t)interp_get_accumulator(interp0, 1)) >> UV_LERP_SHIFT;
            int32_t Zcur = (int32_t)interp_get_accumulator(interp1, 0);
            dst[i] = texturing_power_of_two(texture, textureWidth, textureHeight, textureWidthShift, textureHeightShift, Ucur, Vcur, Zcur, transparent);
            interp_add_accumulator(interp0, 0, (uint32_t)dUdx);
            interp_add_accumulator(interp0, 1, (uint32_t)dVdx);
            interp_add_accumulator(interp1, 0, (uint32_t)dZdx);
        }
    }
    else
    {
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Ucur = ((int32_t)interp_get_accumulator(interp0, 0)) >> UV_LERP_SHIFT;
            int32_t Vcur = ((int32_t)interp_get_accumulator(interp0, 1)) >> UV_LERP_SHIFT;
            int32_t Zcur = (int32_t)interp_get_accumulator(interp1, 0);
            dst[i] = texturing_generic(texture, textureWidth, textureHeight, Ucur, Vcur, Zcur, transparent);
            interp_add_accumulator(interp0, 0, (uint32_t)dUdx);
            interp_add_accumulator(interp0, 1, (uint32_t)dVdx);
            interp_add_accumulator(interp1, 0, (uint32_t)dZdx);
        }
    }
#endif
}

static void shade_span(uint16_t *dst, uint16_t length, Light *light, int32_t L, int32_t dLdx)
{
    if (length <= MAX_SHADING_SPAN_LEN)
    {
#if defined(EUZEBIA3D_PLATFORM_WINDOWS)
        int32_t Lacc = L;
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Lcur = Lacc >> LIGHT_LERP_SHIFT;
            shading(&dst[i], light, Lcur);
            Lacc += dLdx;
        }
#else
        interp_set_accumulator(interp1, 1, (uint32_t)L);

        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Lcur = ((int32_t)interp_get_accumulator(interp1, 1)) >> LIGHT_LERP_SHIFT;
            shading(&dst[i], light, Lcur);
            interp_add_accumulator(interp1, 1, (uint32_t)dLdx);
        }
#endif
    }
    else
    {
        int32_t firstPixelLight = L >> LIGHT_LERP_SHIFT;
        shading(&dst[0], light, firstPixelLight);
    }
}

typedef struct
{
    int32_t L0;
    int32_t L1;
    int32_t U0;
    int32_t U1;
    int32_t V0;
    int32_t V1;
    int32_t Z0;
    int32_t Z1;
} SpanEndpoints;

typedef struct
{
    int32_t L;
    int32_t dLdx;
    int32_t U;
    int32_t dUdx;
    int32_t V;
    int32_t dVdx;
    int32_t Z;
    int32_t dZdx;
} SpanLerpState;

static SpanLerpState make_span_lerp_state(int32_t x0, int32_t x_start, int32_t x_end, const SpanEndpoints *endpoints)
{
    SpanLerpState state = {0};
    int32_t span = x_end - x_start;
    int32_t x_offset = x0 - x_start;

    state.dLdx = span ? (endpoints->L1 - endpoints->L0) / span : 0;
    state.dUdx = span ? (endpoints->U1 - endpoints->U0) / span : 0;
    state.dVdx = span ? (endpoints->V1 - endpoints->V0) / span : 0;
    state.dZdx = span ? (endpoints->Z1 - endpoints->Z0) / span : 0;

    state.L = endpoints->L0 + x_offset * state.dLdx;
    state.U = endpoints->U0 + x_offset * state.dUdx;
    state.V = endpoints->V0 + x_offset * state.dVdx;
    state.Z = endpoints->Z0 + x_offset * state.dZdx;

    return state;
}

static void build_material_span(uint16_t *dst, uint16_t length, Material *mat, Light *light, const SpanLerpState *lerp)
{
    if (length == 0)
        return;

    if (mat->texture == NULL || mat->textureWidth <= 0 || mat->textureHeight <= 0)
        fill_span(dst, length, mat->diffuse);
    else
        texture_span(dst, length, mat, lerp->U, lerp->dUdx, lerp->V, lerp->dVdx, lerp->Z, lerp->dZdx);

#ifdef SHADING_ENABLED
    shade_span(dst, length, light, lerp->L, lerp->dLdx);
#endif
}

inline int32_t calc_pixel_depth(int32_t Ba, int32_t Bb, int32_t Bc, int32_t z1, int32_t z2, int32_t z3)
{
    int32_t z = fixed_mul(Ba, z1) + fixed_mul(Bb, z2) + fixed_mul(Bc, z3);
    return inverse(z);
}

void rasterize(int32_t y, int32_t x0, int32_t x1, Material *mat, Light *light, int32_t L0, int32_t L1, int32_t U0, int32_t U1, int32_t V0, int32_t V1, int32_t Z0, int32_t Z1)
{
    // Scanline rasterizer: barycentrics per line, then per-pixel interpolation
    if (y < 0 || y >= render_height)
        return;

    RENDERER_SET_DEBUG_STAGE(500);
    renderer_debug_vertex_index = (uint32_t)y;

    int32_t n = (y & 1) >> 1;
    x0 += n;
    x1 += n;
    int32_t q;
    if (x1 < x0)
    {
        q = x0;
        x0 = x1;
        x1 = q;
        q = L0;
        L0 = L1;
        L1 = q;
        q = U0;
        U0 = U1;
        U1 = q;
        q = V0;
        V0 = V1;
        V1 = q;
        q = Z0;
        Z0 = Z1;
        Z1 = q;
    }
    x1 += 1;
    int32_t xStart = x0;
    int32_t xEnd = x1;
    if (xEnd < 0 || xStart >= render_width || xStart == xEnd)
        return;
    if (x0 < 0)
        x0 = 0;
    if (x1 > render_width)
        x1 = render_width;
    if (x1 <= x0)
        return;
    uint16_t spanX0 = x0 * output_scale;
    uint16_t spanY = y * output_scale;

    span_length = (uint16_t)(x1 - x0);
    if (span_length > SPAN_BUFFER_MAX)
        span_length = SPAN_BUFFER_MAX;

    SpanEndpoints endpoints = {
        .L0 = L0,
        .L1 = L1,
        .U0 = U0,
        .U1 = U1,
        .V0 = V0,
        .V1 = V1,
        .Z0 = Z0,
        .Z1 = Z1,
    };
    SpanLerpState lerp = make_span_lerp_state(x0, xStart, xEnd, &endpoints);

    RENDERER_SET_DEBUG_STAGE(510);
    build_material_span(span_buffer, span_length, mat, light, &lerp);

    if (span_length > 0)
    {
        const uint16_t *span_to_draw = span_buffer;
        uint16_t span_to_draw_length = span_length;

        if (output_scale > 1)
        {
            uint16_t scaled_length = 0;
            for (uint16_t i = 0; i < span_length; i++)
            {
                for (uint8_t sx = 0; sx < output_scale; sx++)
                {
                    if (scaled_length >= SPAN_BUFFER_MAX)
                        break;
                    span_scaled_buffer[scaled_length++] = span_buffer[i];
                }
                if (scaled_length >= SPAN_BUFFER_MAX)
                    break;
            }
            span_to_draw = span_scaled_buffer;
            span_to_draw_length = scaled_length;
        }

        for (uint8_t dy = 0; dy < output_scale; dy++)
        {
            RENDERER_SET_DEBUG_STAGE(520);
            renderer_debug_scene_counter = span_to_draw_length;
            if (mat->transparent)
                draw_masked_span(spanX0, spanY + dy, span_to_draw, span_to_draw_length);
            else
                _painter->draw_span(spanX0, spanY + dy, span_to_draw, span_to_draw_length);
        }
    }

    RENDERER_SET_DEBUG_STAGE(590);
}

inline void swap_int32(int32_t *x, int32_t *y)
{
    int32_t temp = *x;
    *x = *y;
    *y = temp;
}

void tri(TriangleToRender *triangle, Material *mat, int32_t lightDistances[], Light *light)
{
    RENDERER_SET_DEBUG_STAGE(400);
    prepare_texture_cache(mat);
    RENDERER_SET_DEBUG_STAGE(401);

    int32_t x, y, Lx, Ux, Vx, Zx;
    if (triangle->a.y > triangle->b.y)
    {
        swap_int32(&triangle->a.z, &triangle->b.z);
        swap_int32(&triangle->a.y, &triangle->b.y);
        swap_int32(&triangle->a.x, &triangle->b.x);

        swap_int32(&triangle->uvA.x, &triangle->uvB.x);
        swap_int32(&triangle->uvA.y, &triangle->uvB.y);

        swap_int32(&lightDistances[0], &lightDistances[1]);
    }
    if (triangle->a.y > triangle->c.y)
    {
        swap_int32(&triangle->a.z, &triangle->c.z);
        swap_int32(&triangle->a.y, &triangle->c.y);
        swap_int32(&triangle->a.x, &triangle->c.x);

        swap_int32(&triangle->uvA.x, &triangle->uvC.x);
        swap_int32(&triangle->uvA.y, &triangle->uvC.y);

        swap_int32(&lightDistances[0], &lightDistances[2]);
    }
    if (triangle->b.y > triangle->c.y)
    {
        swap_int32(&triangle->b.z, &triangle->c.z);
        swap_int32(&triangle->b.y, &triangle->c.y);
        swap_int32(&triangle->b.x, &triangle->c.x);

        swap_int32(&triangle->uvB.x, &triangle->uvC.x);
        swap_int32(&triangle->uvB.y, &triangle->uvC.y);

        swap_int32(&lightDistances[1], &lightDistances[2]);
    }
    if (triangle->c.y < 0 || triangle->a.y >= render_height)
    {
        RENDERER_SET_DEBUG_STAGE(402);
        return;
    }

    RENDERER_SET_DEBUG_STAGE(410);
    y = triangle->a.y;
    int32_t xx = x = triangle->a.x;
    int32_t Lxx = Lx = ((int32_t)lightDistances[0]) << LIGHT_LERP_SHIFT;
    int32_t Uxx = Ux = shift_left_clamped_i32(triangle->uvA.x, UV_LERP_SHIFT);
    int32_t Vxx = Vx = shift_left_clamped_i32(triangle->uvA.y, UV_LERP_SHIFT);
    int32_t Zxx = Zx = triangle->a.z;

    int32_t dx01 = triangle->b.x - triangle->a.x;
    int32_t dy01 = triangle->b.y - triangle->a.y;

    int32_t dx02 = triangle->c.x - triangle->a.x;
    int32_t dy02 = triangle->c.y - triangle->a.y;

    int32_t dx12 = triangle->c.x - triangle->b.x;
    int32_t dy12 = triangle->c.y - triangle->b.y;

    int32_t dL01 = dy01 ? ((lightDistances[1] - lightDistances[0]) << LIGHT_LERP_SHIFT) / dy01 : 0;
    int32_t dL02 = dy02 ? ((lightDistances[2] - lightDistances[0]) << LIGHT_LERP_SHIFT) / dy02 : 0;
    int32_t dL12 = dy12 ? ((lightDistances[2] - lightDistances[1]) << LIGHT_LERP_SHIFT) / dy12 : 0;

    int32_t dU01 = shifted_delta_i32(triangle->uvA.x, triangle->uvB.x, UV_LERP_SHIFT, dy01);
    int32_t dV01 = shifted_delta_i32(triangle->uvA.y, triangle->uvB.y, UV_LERP_SHIFT, dy01);
    int32_t dU02 = shifted_delta_i32(triangle->uvA.x, triangle->uvC.x, UV_LERP_SHIFT, dy02);
    int32_t dV02 = shifted_delta_i32(triangle->uvA.y, triangle->uvC.y, UV_LERP_SHIFT, dy02);
    int32_t dU12 = shifted_delta_i32(triangle->uvB.x, triangle->uvC.x, UV_LERP_SHIFT, dy12);
    int32_t dV12 = shifted_delta_i32(triangle->uvB.y, triangle->uvC.y, UV_LERP_SHIFT, dy12);

    int32_t dZ01 = dy01 ? (triangle->b.z - triangle->a.z) / dy01 : 0;
    int32_t dZ02 = dy02 ? (triangle->c.z - triangle->a.z) / dy02 : 0;
    int32_t dZ12 = dy12 ? (triangle->c.z - triangle->b.z) / dy12 : 0;

    int32_t q2 = 0;

    int32_t xxd = 1 - ((dx02 < 0) << 1);

    if (triangle->a.y < triangle->b.y)
    {
        RENDERER_SET_DEBUG_STAGE(430);
        int32_t q = 0;
        int32_t xd = 1 - ((dx01 < 0) << 1);
        while (y <= triangle->b.y && y < render_height)
        {
            RENDERER_SET_DEBUG_STAGE(431);
            renderer_debug_vertex_index = (uint32_t)y;
            rasterize(y, x, xx, mat, light, Lx, Lxx, Ux, Uxx, Vx, Vxx, Zx, Zxx);
            RENDERER_SET_DEBUG_STAGE(432);
            y += 1;
            q += dx01;
            q2 += dx02;
            Lx += dL01;
            Lxx += dL02;
            Ux += dU01;
            Uxx += dU02;
            Vx += dV01;
            Vxx += dV02;
            Zx += dZ01;
            Zxx += dZ02;
            RENDERER_SET_DEBUG_STAGE(433);
            while (xd * q >= dy01)
            {
                q -= xd * dy01;
                x += xd;
            }
            RENDERER_SET_DEBUG_STAGE(434);
            while (xxd * q2 >= dy02)
            {
                q2 -= xxd * dy02;
                xx += xxd;
            }
        }
    }

    if (triangle->b.y < triangle->c.y)
    {
        RENDERER_SET_DEBUG_STAGE(440);
        int32_t q = 0;
        x = triangle->b.x;
        Lx = ((int32_t)lightDistances[1]) << LIGHT_LERP_SHIFT;
        Ux = shift_left_clamped_i32(triangle->uvB.x, UV_LERP_SHIFT);
        Vx = shift_left_clamped_i32(triangle->uvB.y, UV_LERP_SHIFT);
        Zx = triangle->b.z;
        int32_t xd = 1 - ((dx12 < 0) << 1);

        while (y <= triangle->c.y && y < render_height)
        {
            RENDERER_SET_DEBUG_STAGE(441);
            renderer_debug_vertex_index = (uint32_t)y;
            rasterize(y, x, xx, mat, light, Lx, Lxx, Ux, Uxx, Vx, Vxx, Zx, Zxx);
            RENDERER_SET_DEBUG_STAGE(442);
            y += 1;
            q += dx12;
            q2 += dx02;
            Lx += dL12;
            Lxx += dL02;
            Ux += dU12;
            Uxx += dU02;
            Vx += dV12;
            Vxx += dV02;
            Zx += dZ12;
            Zxx += dZ02;
            RENDERER_SET_DEBUG_STAGE(443);
            while (xd * q > dy12)
            {
                q -= xd * dy12;
                x += xd;
            }
            RENDERER_SET_DEBUG_STAGE(444);
            while (xxd * q2 > dy02)
            {
                q2 -= xxd * dy02;
                xx += xxd;
            }
        }
    }

    RENDERER_SET_DEBUG_STAGE(490);
}

typedef struct
{
    uint16_t index;
    int32_t depth;
} SceneDrawItem;
static SceneDrawItem drawItems[MAX_TRIANGLES_IN_SCENE];

static int compare_scene_depth_desc(const void *a, const void *b)
{
    const SceneDrawItem *ta = (const SceneDrawItem *)a;
    const SceneDrawItem *tb = (const SceneDrawItem *)b;
    if (ta->depth < tb->depth)
        return 1;
    if (ta->depth > tb->depth)
        return -1;
    return 0;
}

void render_scene(Light *pLight)
{
    RENDERER_SET_DEBUG_STAGE(300);
    renderer_debug_scene_counter = sceneCounter;

    if (sceneCounter == 0)
    {
        RENDERER_SET_DEBUG_STAGE(301);
        return;
    }

    RENDERER_SET_DEBUG_STAGE(310);
    for (uint16_t i = 0; i < sceneCounter; i++)
    {
        renderer_debug_vertex_index = i;
        const TriangleInScene *triScene = &scene[i];
        drawItems[i].index = i;
        drawItems[i].depth = (triScene->TriangleOnScreen.a.z + triScene->TriangleOnScreen.b.z + triScene->TriangleOnScreen.c.z) / 3;
    }

    RENDERER_SET_DEBUG_STAGE(320);
    if (sceneCounter > 1)
        qsort(drawItems, sceneCounter, sizeof(SceneDrawItem), compare_scene_depth_desc);

    RENDERER_SET_DEBUG_STAGE(330);
    for (uint16_t i = 0; i < sceneCounter; i++)
    {
        RENDERER_SET_DEBUG_STAGE(331);
        renderer_debug_vertex_index = i;
        renderer_debug_face_index = drawItems[i].index;
        const TriangleInScene *triScene = &scene[drawItems[i].index];
        if (triScene->TriangleOnScreen.a.z < NEAR_CLIP_Z || triScene->TriangleOnScreen.b.z < NEAR_CLIP_Z || triScene->TriangleOnScreen.c.z < NEAR_CLIP_Z)
            continue;

        RENDERER_SET_DEBUG_STAGE(340);
        int32_t aW = inverse(triScene->TriangleOnScreen.a.z);
        int32_t bW = inverse(triScene->TriangleOnScreen.b.z);
        int32_t cW = inverse(triScene->TriangleOnScreen.c.z);
        TriangleToRender triangle =
            {
                {
                    triScene->TriangleOnScreen.a.x,
                    triScene->TriangleOnScreen.a.y,
                    triScene->TriangleOnScreen.a.z,
                    aW,
                },
                {
                    triScene->TriangleOnScreen.b.x,
                    triScene->TriangleOnScreen.b.y,
                    triScene->TriangleOnScreen.b.z,
                    bW,
                },
                {
                    triScene->TriangleOnScreen.c.x,
                    triScene->TriangleOnScreen.c.y,
                    triScene->TriangleOnScreen.c.z,
                    cW,
                },
                {
                    make_perspective_uv(triScene->UV.a.x, aW),
                    make_perspective_uv(triScene->UV.a.y, aW),
                },
                {
                    make_perspective_uv(triScene->UV.b.x, bW),
                    make_perspective_uv(triScene->UV.b.y, bW),
                },
                {
                    make_perspective_uv(triScene->UV.c.x, cW),
                    make_perspective_uv(triScene->UV.c.y, cW),
                },
            };

        int32_t lightDistances[3] = {0, 0, 0};
#ifdef SHADING_ENABLED
        lightDistances[0] = triScene->LightDistances[0];
        lightDistances[1] = triScene->LightDistances[1];
        lightDistances[2] = triScene->LightDistances[2];
#endif

        RENDERER_SET_DEBUG_STAGE(350);
        tri(&triangle, triScene->mat, lightDistances, pLight);
    }

    RENDERER_SET_DEBUG_STAGE(390);
}

void add_model_to_scene(Mesh *mesh, Camera *camera, Light *pLight)
{
    RENDERER_SET_DEBUG_STAGE(100);
    renderer_debug_vertex_index = 0;
    renderer_debug_face_index = 0;
    renderer_debug_scene_counter = sceneCounter;

    if (mesh == NULL || camera == NULL || pLight == NULL || mesh->mat == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(101);
        return;
    }

    uint16_t verticesCounter = mesh->verticesCounter;
    uint16_t textureCoordsCounter = mesh->textureCoordsCounter;
    uint16_t vnCounter = mesh->vnCounter;
    uint32_t transformationsNum = mesh->transformationsNum;
    renderer_debug_transform_count = transformationsNum;
    renderer_debug_pointer = (uintptr_t)mesh->transformations;

    if (mesh->vertices == NULL || mesh->faces == NULL || mesh->textureCoords == NULL || mesh->uv == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(102);
        return;
    }

    if (verticesCounter == 0 || mesh->facesCounter == 0 || textureCoordsCounter == 0)
    {
        RENDERER_SET_DEBUG_STAGE(103);
        return;
    }

    if (mesh->vn == NULL || mesh->normals == NULL || vnCounter == 0)
    {
        RENDERER_SET_DEBUG_STAGE(104);
        return;
    }

    if (transformationsNum > 0 && mesh->transformations == NULL)
    {
        RENDERER_SET_DEBUG_STAGE(106);
        return;
    }

    if (transformationsNum > MAX_MODEL_TRANSFORMATIONS)
    {
        RENDERER_SET_DEBUG_STAGE(107);
        return;
    }

    if (!ensure_model_scratch_capacity(verticesCounter, vnCounter))
    {
        RENDERER_SET_DEBUG_STAGE(105);
        return;
    }

    int32_t *verticesModified = modelScratchVerticesModified;
    int32_t *verticesClip = modelScratchVerticesClip;
    int32_t *normalsModified = modelScratchNormalsModified;

    RENDERER_SET_DEBUG_STAGE(110);
    memcpy(verticesModified, mesh->vertices, verticesCounter * 3 * sizeof(int32_t));
    memcpy(normalsModified, mesh->vn, vnCounter * 3 * sizeof(int32_t));

    RENDERER_SET_DEBUG_STAGE(120);
    for (uint32_t i = 0; i < transformationsNum; i++)
    {
        renderer_debug_vertex_index = i;
        renderer_debug_pointer = (uintptr_t)&mesh->transformations[i];
        RENDERER_SET_DEBUG_STAGE(121);
        transform(verticesModified, verticesCounter, &mesh->transformations[i]);
        RENDERER_SET_DEBUG_STAGE(122);
        if (mesh->transformations[i].transformType != MODEL_TRANSFORM_TRANSLATE)
        {
            RENDERER_SET_DEBUG_STAGE(123);
            // Normals are directions; translation must not affect them.
            transform(normalsModified, vnCounter, &mesh->transformations[i]);
            RENDERER_SET_DEBUG_STAGE(124);
        }
    }

    RENDERER_SET_DEBUG_STAGE(130);
    for (uint16_t i = 0; i < verticesCounter * 3; i += 3)
    {
        renderer_debug_vertex_index = (uint32_t)i / 3u;
        int32_t x = verticesModified[i];
        int32_t y = verticesModified[i + 1];
        int32_t z = verticesModified[i + 2];
        int32_t w = SCALE_FACTOR;
        fixed_mul_matrix_vector(&x, &y, &z, &w, camera->vMatrix);
        fixed_mul_matrix_vector(&x, &y, &z, &w, camera->pMatrix);
        size_t clipBase = ((size_t)i / 3u) * 4u;
        verticesClip[clipBase] = x;
        verticesClip[clipBase + 1] = y;
        verticesClip[clipBase + 2] = z;
        verticesClip[clipBase + 3] = w;
    }

    RENDERER_SET_DEBUG_STAGE(135);
    for (uint16_t i = 0; i < vnCounter; i++)
    {
        renderer_debug_vertex_index = i;
        size_t base = (size_t)i * 3u;
        Vector3 normal = {
            .x = normalsModified[base],
            .y = normalsModified[base + 1u],
            .z = normalsModified[base + 2u],
        };

        if (!norm_vector_safe(&normal))
        {
            normal.x = 0;
            normal.y = 0;
            normal.z = 0;
        }
        write_vector_triplet(normalsModified, i, &normal);
    }

    RENDERER_SET_DEBUG_STAGE(136);
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        renderer_debug_vertex_index = i;
        size_t base = (size_t)i * 3u;
        Vector3 lightDirection;
        make_light_direction(&lightDirection, pLight, verticesModified, base);
        write_vector_triplet(verticesModified, i, &lightDirection);
    }

    Vector3 normalVectorA;
    Vector3 normalVectorB;
    Vector3 normalVectorC;
    Vector3 lightDirectionA;
    Vector3 lightDirectionB;
    Vector3 lightDirectionC;

    RENDERER_SET_DEBUG_STAGE(140);
    uint32_t faceIndexCount = (uint32_t)mesh->facesCounter * 3u;
    for (uint32_t i = 0; i < faceIndexCount; i += 3u)
    {
        RENDERER_SET_DEBUG_STAGE(150);
        renderer_debug_face_index = i / 3u;
        renderer_debug_scene_counter = sceneCounter;

        uint16_t a = mesh->faces[i];
        uint16_t b = mesh->faces[i + 1];
        uint16_t c = mesh->faces[i + 2];
        uint16_t uvA = mesh->uv[i];
        uint16_t uvB = mesh->uv[i + 1];
        uint16_t uvC = mesh->uv[i + 2];

        if (a >= verticesCounter || b >= verticesCounter || c >= verticesCounter)
        {
            RENDERER_SET_DEBUG_STAGE(151);
            continue;
        }

        if (uvA >= textureCoordsCounter || uvB >= textureCoordsCounter || uvC >= textureCoordsCounter)
        {
            RENDERER_SET_DEBUG_STAGE(152);
            continue;
        }

        if (mesh->normals[i] >= vnCounter || mesh->normals[i + 1] >= vnCounter || mesh->normals[i + 2] >= vnCounter)
        {
            RENDERER_SET_DEBUG_STAGE(153);
            continue;
        }

        int32_t lightDistances[3] = {0, 0, 0};
        RENDERER_SET_DEBUG_STAGE(160);
        renderer_debug_vertex_index = mesh->normals[i];
        if (!read_normal_vector(&normalVectorA, normalsModified, vnCounter, mesh->normals[i]) ||
            !read_normal_vector(&normalVectorB, normalsModified, vnCounter, mesh->normals[i + 1]) ||
            !read_normal_vector(&normalVectorC, normalsModified, vnCounter, mesh->normals[i + 2]))
        {
            lightDistances[0] = 0;
            lightDistances[1] = 0;
            lightDistances[2] = 0;
        }
        else
        {
            RENDERER_SET_DEBUG_STAGE(163);
            lightDirectionA.x = verticesModified[a * 3];
            lightDirectionA.y = verticesModified[a * 3 + 1];
            lightDirectionA.z = verticesModified[a * 3 + 2];
            lightDirectionB.x = verticesModified[b * 3];
            lightDirectionB.y = verticesModified[b * 3 + 1];
            lightDirectionB.z = verticesModified[b * 3 + 2];
            lightDirectionC.x = verticesModified[c * 3];
            lightDirectionC.y = verticesModified[c * 3 + 1];
            lightDirectionC.z = verticesModified[c * 3 + 2];

            RENDERER_SET_DEBUG_STAGE(165);
            lightDistances[0] = clamp_light_distance(dot_product(&normalVectorA, &lightDirectionA));
            lightDistances[1] = clamp_light_distance(dot_product(&normalVectorB, &lightDirectionB));
            lightDistances[2] = clamp_light_distance(dot_product(&normalVectorC, &lightDirectionC));
        }

        ClipVertex triangleIn[3] = {
            {
                .x = verticesClip[a * 4],
                .y = verticesClip[a * 4 + 1],
                .z = verticesClip[a * 4 + 2],
                .w = verticesClip[a * 4 + 3],
                .uvx = mesh->textureCoords[uvA * 2],
                .uvy = mesh->textureCoords[uvA * 2 + 1],
                .light = lightDistances[0],
            },
            {
                .x = verticesClip[b * 4],
                .y = verticesClip[b * 4 + 1],
                .z = verticesClip[b * 4 + 2],
                .w = verticesClip[b * 4 + 3],
                .uvx = mesh->textureCoords[uvB * 2],
                .uvy = mesh->textureCoords[uvB * 2 + 1],
                .light = lightDistances[1],
            },
            {
                .x = verticesClip[c * 4],
                .y = verticesClip[c * 4 + 1],
                .z = verticesClip[c * 4 + 2],
                .w = verticesClip[c * 4 + 3],
                .uvx = mesh->textureCoords[uvC * 2],
                .uvy = mesh->textureCoords[uvC * 2 + 1],
                .light = lightDistances[2],
            }};

        RENDERER_SET_DEBUG_STAGE(170);
        ClipVertex clipped[4];
        uint8_t clippedCount = clip_triangle_against_near_plane(triangleIn, clipped);
        if (clippedCount < 3)
            continue;

        for (uint8_t t = 1; t + 1 < clippedCount; t++)
        {
            ClipVertex va = clipped[0];
            ClipVertex vb = clipped[t];
            ClipVertex vc = clipped[t + 1];

            if (va.w <= 0 || vb.w <= 0 || vc.w <= 0)
                continue;

            RENDERER_SET_DEBUG_STAGE(180);
            int32_t ax = fixed_div(va.x, va.w) + render_width_half;
            int32_t ay = fixed_div(va.y, va.w) + render_height_half;
            int32_t bx = fixed_div(vb.x, vb.w) + render_width_half;
            int32_t by = fixed_div(vb.y, vb.w) + render_height_half;
            int32_t cx = fixed_div(vc.x, vc.w) + render_width_half;
            int32_t cy = fixed_div(vc.y, vc.w) + render_height_half;

            if (triangle_outside_render_area(ax, ay, bx, by, cx, cy))
                continue;

            if (triangle_outside_raster_guard(ax, ay, bx, by, cx, cy))
                continue;

            Triangle2D triangle = {
                {ax, ay},
                {bx, by},
                {cx, cy},
            };

            if (!check_if_triangle_visible(&triangle))
                continue;

            if (sceneCounter >= MAX_TRIANGLES_IN_SCENE)
            {
                RENDERER_SET_DEBUG_STAGE(190);
                return;
            }

            RENDERER_SET_DEBUG_STAGE(191);
            TriangleInScene *outTriangle = &scene[sceneCounter++];
            outTriangle->TriangleOnScreen.a.x = ax;
            outTriangle->TriangleOnScreen.a.y = ay;
            outTriangle->TriangleOnScreen.a.z = va.z;
            outTriangle->TriangleOnScreen.b.x = bx;
            outTriangle->TriangleOnScreen.b.y = by;
            outTriangle->TriangleOnScreen.b.z = vb.z;
            outTriangle->TriangleOnScreen.c.x = cx;
            outTriangle->TriangleOnScreen.c.y = cy;
            outTriangle->TriangleOnScreen.c.z = vc.z;

            outTriangle->UV.a.x = va.uvx;
            outTriangle->UV.a.y = va.uvy;
            outTriangle->UV.b.x = vb.uvx;
            outTriangle->UV.b.y = vb.uvy;
            outTriangle->UV.c.x = vc.uvx;
            outTriangle->UV.c.y = vc.uvy;

            outTriangle->LightDistances[0] = va.light;
            outTriangle->LightDistances[1] = vb.light;
            outTriangle->LightDistances[2] = vc.light;
            outTriangle->mat = mesh->mat;
        }
    }

    RENDERER_SET_DEBUG_STAGE(200);
    renderer_debug_scene_counter = sceneCounter;
}

void clean_scene()
{
    sceneCounter = 0;
}

static IRenderer renderer = {
    .init_renderer = init_renderer,
    .render_scene = render_scene,
    .add_model_to_scene = add_model_to_scene,
    .clean_scene = clean_scene,
    .set_scale = renderer_set_scale};

const IRenderer *get_renderer(void)
{
    return &renderer;
}
