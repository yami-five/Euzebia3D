#include "IRenderer.h"
#include "renderer.h"
#include "hardware/interp.h"
#include <stdlib.h>

static volatile const IHardware *_hardware = NULL;
static volatile const IPainter *_painter = NULL;

static const uint8_t FOCAL_LENGTH = 90;
static const uint32_t FIRE_FLOOR_ADR = 76480;
static const uint32_t FIXED_FOCAL_LENGTH = 90 << SHIFT_FACTOR;
static const uint32_t TRIANGLE_CENTER_DIVIDER = 3 << SHIFT_FACTOR;
static const uint16_t BASE_WIDTH = 320;
static const uint16_t BASE_HEIGHT = 240;

static uint8_t render_scale = 2; // 2 => 160x120 render; 1 => 320x240 render
static uint8_t output_scale = 2;
static uint16_t render_width = 160;
static uint16_t render_height = 120;
static uint16_t render_width_half = 80;
static uint16_t render_height_half = 60;

#define MAX_TRIANGLES_IN_SCENE 1500
#define SPAN_BUFFER_MAX 320
#define SHADING_ENABLED 1
#define LIGHT_LERP_SHIFT 8
#define UV_LERP_SHIFT 8
#define MAX_SHADING_SPAN_LEN 240
// Render can be downscaled: render_scale=2 -> 160x120 rendered, scaled to LCD in painter.

static TriangleInScene scene[MAX_TRIANGLES_IN_SCENE];
static uint16_t sceneCounter = 0;
static uint16_t span_buffer[SPAN_BUFFER_MAX];
static uint16_t span_scaled_buffer[SPAN_BUFFER_MAX];
static uint16_t span_length = 0;

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

void init_renderer(volatile const IHardware *hardware, volatile const IPainter *painter)
{
    _hardware = hardware;
    _painter = painter;
    configure_render_dimensions();
    init_span_interpolators();
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
    int32_t qt_rad = fixed_mul(vector->w, PI2);
    int32_t c = fast_cos(qt_rad >> 1);
    int32_t s = fast_sin(qt_rad >> 1);
    Vector3 qVec = {
        .x = vector->x,
        .y = vector->y,
        .z = vector->z};
    Quaternion q = {
        .w = c,
        .vec = &qVec};
    norm_vector(q.vec);
    mul_vec_scalar(q.vec, s);
    Vector3 qVecInv = {
        .x = -q.vec->x,
        .y = -q.vec->y,
        .z = -q.vec->z};
    Quaternion qInv = {
        .w = c,
        .vec = &qVecInv};
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
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
}

void translate(int32_t *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i * 3] += vector->x;
        vertices[i * 3 + 1] += vector->y;
        vertices[i * 3 + 2] += vector->z;
    }
}

void scale(int32_t *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i * 3] = fixed_mul(vertices[i * 3], vector->x);
        vertices[i * 3 + 1] = fixed_mul(vertices[i * 3 + 1], vector->y);
        vertices[i * 3 + 2] = fixed_mul(vertices[i * 3 + 2], vector->z);
    }
}

void transform(int32_t *vertices, uint16_t verticesCounter, TransformInfo *transformInfo)
{
    if (transformInfo->transformType == MODEL_TRANSFORM_ROTATE)
        rotate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == MODEL_TRANSFORM_TRANSLATE)
        translate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == MODEL_TRANSFORM_SCALE)
        scale(vertices, verticesCounter, transformInfo->transformVector);
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

    return (e1x * e2y - e1y * e2x) >= 0;
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
    return v->z > 0;
}

static inline int32_t interpolate_fixed(int32_t a, int32_t b, int32_t t)
{
    return a + (int32_t)fixed_mul((b - a), t);
}

static ClipVertex intersect_near_plane(const ClipVertex *a, const ClipVertex *b)
{
    const int32_t NEAR_CLIP_Z = 1;
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

void shading(uint16_t *color, PointLight *light, int32_t lightDistance)
{
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

uint16_t texturing(Material *mat, int32_t U, int32_t V, int32_t Z)
{
    int32_t uv_x = (int32_t)fixed_mul(U, Z);
    int32_t uv_y = (int32_t)fixed_mul(V, Z);
    uv_x = uv_x * mat->textureSize >> SHIFT_FACTOR;
    uv_y = uv_y * mat->textureSize >> SHIFT_FACTOR;
    if (uv_x < 1)
        uv_x = 1;
    if (uv_y < 1)
        uv_y = 1;
    if (uv_x > mat->textureSize - 2)
        uv_x = mat->textureSize - 2;
    if (uv_y > mat->textureSize - 2)
        uv_y = mat->textureSize - 2;
    int32_t x0 = uv_x;
    int32_t y0 = uv_y;
    int32_t x1 = x0 + 1;
    int32_t y1 = y0 + 1;

    uint16_t c00 = mat->texture[y0 * mat->textureSize + x0];
    uint16_t c10 = mat->texture[y0 * mat->textureSize + x1];
    uint16_t c01 = mat->texture[y1 * mat->textureSize + x0];
    uint16_t c11 = mat->texture[y1 * mat->textureSize + x1];

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

static inline void fill_span(uint16_t *dst, uint16_t length, uint16_t color)
{
    for (uint16_t i = 0; i < length; i++)
        dst[i] = color;
}

static void texture_span(uint16_t *dst, uint16_t length, Material *mat, int32_t U, int32_t dUdx, int32_t V, int32_t dVdx, int32_t Z, int32_t dZdx)
{
    interp_set_accumulator(interp0, 0, (uint32_t)U);
    interp_set_accumulator(interp0, 1, (uint32_t)V);
    interp_set_accumulator(interp1, 0, (uint32_t)Z);

    for (uint16_t i = 0; i < length; i++)
    {
        int32_t Ucur = ((int32_t)interp_get_accumulator(interp0, 0)) >> UV_LERP_SHIFT;
        int32_t Vcur = ((int32_t)interp_get_accumulator(interp0, 1)) >> UV_LERP_SHIFT;
        int32_t Zcur = (int32_t)interp_get_accumulator(interp1, 0);
        dst[i] = texturing(mat, Ucur, Vcur, Zcur);
        interp_add_accumulator(interp0, 0, (uint32_t)dUdx);
        interp_add_accumulator(interp0, 1, (uint32_t)dVdx);
        interp_add_accumulator(interp1, 0, (uint32_t)dZdx);
    }
}

static void shade_span(uint16_t *dst, uint16_t length, PointLight *light, int32_t L, int32_t dLdx)
{
    if(length<=MAX_SHADING_SPAN_LEN)
    {
        interp_set_accumulator(interp1, 1, (uint32_t)L);
        
        for (uint16_t i = 0; i < length; i++)
        {
            int32_t Lcur = ((int32_t)interp_get_accumulator(interp1, 1)) >> LIGHT_LERP_SHIFT;
            shading(&dst[i], light, Lcur);
            interp_add_accumulator(interp1, 1, (uint32_t)dLdx);
        }
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

static void build_material_span(uint16_t *dst, uint16_t length, Material *mat, PointLight *light, uint8_t apply_shading, const SpanLerpState *lerp)
{
    if (length == 0)
        return;

    if (mat->textureSize == 0)
        fill_span(dst, length, mat->diffuse);
    else
        texture_span(dst, length, mat, lerp->U, lerp->dUdx, lerp->V, lerp->dVdx, lerp->Z, lerp->dZdx);

    if (apply_shading)
        shade_span(dst, length, light, lerp->L, lerp->dLdx);
}

inline int32_t calc_pixel_depth(int32_t Ba, int32_t Bb, int32_t Bc, int32_t z1, int32_t z2, int32_t z3)
{
    int32_t z = fixed_mul(Ba, z1) + fixed_mul(Bb, z2) + fixed_mul(Bc, z3);
    return inverse(z);
}

void rasterize(int32_t y, int32_t x0, int32_t x1, Material *mat, PointLight *light, int32_t L0, int32_t L1, int32_t U0, int32_t U1, int32_t V0, int32_t V1, int32_t Z0, int32_t Z1)
{
    // Scanline rasterizer: barycentrics per line, then per-pixel interpolation
    if (y < 0 || y >= render_height)
        return;
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
    uint8_t applyShading = (mat->isSkyBox == 0) && SHADING_ENABLED;
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

    build_material_span(span_buffer, span_length, mat, light, applyShading, &lerp);

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
            _painter->draw_span(spanX0, spanY + dy, span_to_draw, span_to_draw_length);
    }
}

inline void swap_int32(int32_t *x, int32_t *y)
{
    int32_t temp = *x;
    *x = *y;
    *y = temp;
}

void tri(TriangleToRender *triangle, Material *mat, int32_t lightDistances[], PointLight *light)
{
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
    if (triangle->c.y < 0 || triangle->a.y > render_height)
        return;
    y = triangle->a.y;
    int32_t xx = x = triangle->a.x;
    int32_t Lxx = Lx = ((int32_t)lightDistances[0]) << LIGHT_LERP_SHIFT;
    int32_t Uxx = Ux = triangle->uvA.x << UV_LERP_SHIFT;
    int32_t Vxx = Vx = triangle->uvA.y << UV_LERP_SHIFT;
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

    int32_t dU01 = dy01 ? ((triangle->uvB.x - triangle->uvA.x) << UV_LERP_SHIFT) / dy01 : 0;
    int32_t dV01 = dy01 ? ((triangle->uvB.y - triangle->uvA.y) << UV_LERP_SHIFT) / dy01 : 0;
    int32_t dU02 = dy02 ? ((triangle->uvC.x - triangle->uvA.x) << UV_LERP_SHIFT) / dy02 : 0;
    int32_t dV02 = dy02 ? ((triangle->uvC.y - triangle->uvA.y) << UV_LERP_SHIFT) / dy02 : 0;
    int32_t dU12 = dy12 ? ((triangle->uvC.x - triangle->uvB.x) << UV_LERP_SHIFT) / dy12 : 0;
    int32_t dV12 = dy12 ? ((triangle->uvC.y - triangle->uvB.y) << UV_LERP_SHIFT) / dy12 : 0;

    int32_t dZ01 = dy01 ? (triangle->b.z - triangle->a.z) / dy01 : 0;
    int32_t dZ02 = dy02 ? (triangle->c.z - triangle->a.z) / dy02 : 0;
    int32_t dZ12 = dy12 ? (triangle->c.z - triangle->b.z) / dy12 : 0;

    int32_t q2 = 0;

    int32_t xxd = 1 - ((dx02 < 0) << 1);

    if (triangle->a.y < triangle->b.y)
    {
        int32_t q = 0;
        int32_t xd = 1 - ((dx01 < 0) << 1);
        while (y <= triangle->b.y)
        {
            rasterize(y, x, xx, mat, light, Lx, Lxx, Ux, Uxx, Vx, Vxx, Zx, Zxx);
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
            while (xd * q >= dy01)
            {
                q -= xd * dy01;
                x += xd;
            }
            while (xxd * q2 >= dy02)
            {
                q2 -= xxd * dy02;
                xx += xxd;
            }
        }
    }

    if (triangle->b.y < triangle->c.y)
    {
        int32_t q = 0;
        x = triangle->b.x;
        Lx = ((int32_t)lightDistances[1]) << LIGHT_LERP_SHIFT;
        Ux = triangle->uvB.x << UV_LERP_SHIFT;
        Vx = triangle->uvB.y << UV_LERP_SHIFT;
        Zx = triangle->b.z;
        int32_t xd = 1 - ((dx12 < 0) << 1);

        while (y <= triangle->c.y && y < render_height)
        {
            rasterize(y, x, xx, mat, light, Lx, Lxx, Ux, Uxx, Vx, Vxx, Zx, Zxx);
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
            while (xd * q > dy12)
            {
                q -= xd * dy12;
                x += xd;
            }
            while (xxd * q2 > dy02)
            {
                q2 -= xxd * dy02;
                xx += xxd;
            }
        }
    }
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

void render_scene(PointLight *pLight)
{
    if (sceneCounter == 0)
        return;

    for (uint16_t i = 0; i < sceneCounter; i++)
    {
        const TriangleInScene *triScene = &scene[i];
        drawItems[i].index = i;
        drawItems[i].depth = (triScene->TriangleOnScreen.a.z + triScene->TriangleOnScreen.b.z + triScene->TriangleOnScreen.c.z) / 3;
    }

    if (sceneCounter > 1)
        qsort(drawItems, sceneCounter, sizeof(SceneDrawItem), compare_scene_depth_desc);

    for (uint16_t i = 0; i < sceneCounter; i++)
    {
        const TriangleInScene *triScene = &scene[drawItems[i].index];
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
                    fixed_mul(triScene->UV.a.x, aW),
                    fixed_mul(triScene->UV.a.y, aW),
                },
                {
                    fixed_mul(triScene->UV.b.x, bW),
                    fixed_mul(triScene->UV.b.y, bW),
                },
                {
                    fixed_mul(triScene->UV.c.x, cW),
                    fixed_mul(triScene->UV.c.y, cW),
                },
            };

        int32_t lightDistances[3] = {0, 0, 0};
#ifdef SHADING_ENABLED
        if (triScene->mat->isSkyBox == 0)
        {
            lightDistances[0] = triScene->LightDistances[0];
            lightDistances[1] = triScene->LightDistances[1];
            lightDistances[2] = triScene->LightDistances[2];
        }
#endif

        tri(&triangle, triScene->mat, lightDistances, pLight);
    }
}

void add_model_to_scene(Mesh *mesh, Camera *camera, PointLight *pLight)
{
    uint16_t verticesCounter = mesh->verticesCounter;
    uint16_t vnCounter = mesh->vnCounter;

    if (!ensure_model_scratch_capacity(verticesCounter, vnCounter))
        return;

    int32_t *verticesModified = modelScratchVerticesModified;
    int32_t *verticesClip = modelScratchVerticesClip;
    int32_t *normalsModified = modelScratchNormalsModified;

    memcpy(verticesModified, mesh->vertices, verticesCounter * 3 * sizeof(int32_t));
    memcpy(normalsModified, mesh->vn, vnCounter * 3 * sizeof(int32_t));
    for (uint32_t i = 0; i < mesh->transformationsNum; i++)
    {
        transform(verticesModified, verticesCounter, &mesh->transformations[i]);
        transform(normalsModified, vnCounter, &mesh->transformations[i]);
    }

    for (uint16_t i = 0; i < verticesCounter * 3; i += 3)
    {
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

    Vector3 normalVectorA;
    Vector3 normalVectorB;
    Vector3 normalVectorC;
    Vector3 lightDirectionA;
    Vector3 lightDirectionB;
    Vector3 lightDirectionC;

    for (uint16_t i = 0; i < mesh->facesCounter * 3; i += 3)
    {
        uint16_t a = mesh->faces[i];
        uint16_t b = mesh->faces[i + 1];
        uint16_t c = mesh->faces[i + 2];
        uint16_t uvA = mesh->uv[i];
        uint16_t uvB = mesh->uv[i + 1];
        uint16_t uvC = mesh->uv[i + 2];

        int32_t lightDistances[3] = {0, 0, 0};
        if (mesh->mat->isSkyBox == 0)
        {
            normalVectorA.x = normalsModified[mesh->normals[i] * 3];
            normalVectorA.y = normalsModified[mesh->normals[i] * 3 + 1];
            normalVectorA.z = normalsModified[mesh->normals[i] * 3 + 2];
            normalVectorB.x = normalsModified[mesh->normals[i + 1] * 3];
            normalVectorB.y = normalsModified[mesh->normals[i + 1] * 3 + 1];
            normalVectorB.z = normalsModified[mesh->normals[i + 1] * 3 + 2];
            normalVectorC.x = normalsModified[mesh->normals[i + 2] * 3];
            normalVectorC.y = normalsModified[mesh->normals[i + 2] * 3 + 1];
            normalVectorC.z = normalsModified[mesh->normals[i + 2] * 3 + 2];

            if (!norm_vector_safe(&normalVectorA) || !norm_vector_safe(&normalVectorB) || !norm_vector_safe(&normalVectorC))
            {
                lightDistances[0] = 0;
                lightDistances[1] = 0;
                lightDistances[2] = 0;
            }
            else
            {
                lightDirectionA.x = pLight->position.x - verticesModified[a * 3];
                lightDirectionA.y = pLight->position.y - verticesModified[a * 3 + 1];
                lightDirectionA.z = pLight->position.z - verticesModified[a * 3 + 2];
                lightDirectionB.x = pLight->position.x - verticesModified[b * 3];
                lightDirectionB.y = pLight->position.y - verticesModified[b * 3 + 1];
                lightDirectionB.z = pLight->position.z - verticesModified[b * 3 + 2];
                lightDirectionC.x = pLight->position.x - verticesModified[c * 3];
                lightDirectionC.y = pLight->position.y - verticesModified[c * 3 + 1];
                lightDirectionC.z = pLight->position.z - verticesModified[c * 3 + 2];

                if (!norm_vector_safe(&lightDirectionA) || !norm_vector_safe(&lightDirectionB) || !norm_vector_safe(&lightDirectionC))
                {
                    lightDistances[0] = 0;
                    lightDistances[1] = 0;
                    lightDistances[2] = 0;
                }
                else
                {
                    lightDistances[0] = dot_product(&normalVectorA, &lightDirectionA);
                    if (lightDistances[0] < 0)
                        lightDistances[0] = 0;
                    if (lightDistances[0] > SCALE_FACTOR)
                        lightDistances[0] = SCALE_FACTOR;

                    lightDistances[1] = dot_product(&normalVectorB, &lightDirectionB);
                    if (lightDistances[1] < 0)
                        lightDistances[1] = 0;
                    if (lightDistances[1] > SCALE_FACTOR)
                        lightDistances[1] = SCALE_FACTOR;

                    lightDistances[2] = dot_product(&normalVectorC, &lightDirectionC);
                    if (lightDistances[2] < 0)
                        lightDistances[2] = 0;
                    if (lightDistances[2] > SCALE_FACTOR)
                        lightDistances[2] = SCALE_FACTOR;
                }
            }
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

        ClipVertex clipped[4];
        uint8_t clippedCount = clip_triangle_against_near_plane(triangleIn, clipped);
        if (clippedCount < 3)
            continue;

        for (uint8_t t = 1; t + 1 < clippedCount; t++)
        {
            ClipVertex va = clipped[0];
            ClipVertex vb = clipped[t];
            ClipVertex vc = clipped[t + 1];

            if (va.w == 0 || vb.w == 0 || vc.w == 0)
                continue;

            int32_t ax = fixed_div(va.x, va.w) + render_width_half;
            int32_t ay = fixed_div(va.y, va.w) + render_height_half;
            int32_t bx = fixed_div(vb.x, vb.w) + render_width_half;
            int32_t by = fixed_div(vb.y, vb.w) + render_height_half;
            int32_t cx = fixed_div(vc.x, vc.w) + render_width_half;
            int32_t cy = fixed_div(vc.y, vc.w) + render_height_half;

            Triangle2D triangle = {
                {ax, ay},
                {bx, by},
                {cx, cy},
            };

            if (!check_if_triangle_visible(&triangle))
                continue;

            if (sceneCounter >= MAX_TRIANGLES_IN_SCENE)
                return;

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
