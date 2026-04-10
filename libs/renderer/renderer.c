#include "IRenderer.h"
#include "renderer.h"
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
#define SHADING_ENABLED 1
#define LIGHT_LERP_SHIFT 8
// Render can be downscaled: render_scale=2 -> 160x120 rendered, scaled to LCD in painter.

static TriangleInScene scene[MAX_TRIANGLES_IN_SCENE];
static uint16_t sceneCounter = 0;

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
    if (transformInfo->transformType == 0)
        rotate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == 1)
        translate(vertices, verticesCounter, transformInfo->transformVector);
    if (transformInfo->transformType == 2)
        scale(vertices, verticesCounter, transformInfo->transformVector);
}

void inf(float *x, float *y, float qt)
{
    float qt_rad = qt * PI2;
    *x += 2.0f * (fast_cos(qt_rad));
    *y += 2.0f * fast_cos(qt_rad) * fast_sin(qt_rad);
}

int check_if_triangle_visible(Triangle2D *triangle)
{
    int e1x = triangle->b.x - triangle->a.x;
    int e1y = triangle->b.y - triangle->a.y;
    int e2x = triangle->c.x - triangle->a.x;
    int e2y = triangle->c.y - triangle->a.y;

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
    // if (lightDistance <= 0)
    // {
    //     *color = 0;
    //     return;
    // }
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

uint16_t texturing(TriangleToRender *triangle, Material *mat, int Ba, int Bb, int Bc)
{
    int uv_x = (fixed_mul(Ba, triangle->uvA.x) + fixed_mul(Bb, triangle->uvB.x) + fixed_mul(Bc, triangle->uvC.x)) * mat->textureSize;
    int uv_y = (fixed_mul(Ba, triangle->uvA.y) + fixed_mul(Bb, triangle->uvB.y) + fixed_mul(Bc, triangle->uvC.y)) * mat->textureSize;
    uv_x += (SCALE_FACTOR >> 1);
    uv_y += (SCALE_FACTOR >> 1);
    uv_x = uv_x >> SHIFT_FACTOR;
    uv_y = uv_y >> SHIFT_FACTOR;
    if (uv_x < 1)
        uv_x = 1;
    if (uv_y < 1)
        uv_y = 1;
    if (uv_x > mat->textureSize - 2)
        uv_x = mat->textureSize - 2;
    if (uv_y > mat->textureSize - 2)
        uv_y = mat->textureSize - 2;
    int x0 = uv_x;
    int y0 = uv_y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;

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

inline int calc_pixel_depth(int Ba, int Bb, int Bc, int z1, int z2, int z3)
{
    int z = fixed_mul(Ba, z1) + fixed_mul(Bb, z2) + fixed_mul(Bc, z3);
    return inverse(z);
}

void calc_bar_coords(TriangleToRender *triangle, int *Ba, int *Bb, int *Bc, int32_t divider, int x, int y)
{
    int tempBa, tempBb, tempBc;
    tempBa = ((triangle->b.y - triangle->c.y) * (x - triangle->c.x) + (triangle->c.x - triangle->b.x) * (y - triangle->c.y)) << SHIFT_FACTOR;
    tempBb = ((triangle->c.y - triangle->a.y) * (x - triangle->c.x) + (triangle->a.x - triangle->c.x) * (y - triangle->c.y)) << SHIFT_FACTOR;
    tempBa = fixed_div(tempBa, divider);
    tempBb = fixed_div(tempBb, divider);
    tempBc = SCALE_FACTOR - tempBa - tempBb;
    *Ba = tempBa;
    *Bb = tempBb;
    *Bc = tempBc;
}

void rasterize(int y, int x0, int x1, TriangleToRender *triangle, Material *mat, int32_t divider, PointLight *light, int32_t L0, int32_t L1)
{
    // Scanline rasterizer: barycentrics per line, then per-pixel interpolation
    if (y < 0 || y >= render_height)
        return;
    int n = (y & 1) >> 1;
    x0 += n;
    x1 += n;
    int q;
    if (x1 < x0)
    {
        q = x0;
        x0 = x1;
        x1 = q;
        int32_t lq = L0;
        L0 = L1;
        L1 = lq;
    }
    x1 += 1;
    int xStart = x0;
    int xEnd = x1;
    if (xEnd < 0 || xStart >= render_width || xStart == xEnd)
        return;
    int span = xEnd - xStart;
    int32_t dLdx = span ? (L1 - L0) / span : 0;
    if (x0 < 0)
        x0 = 0;
    if (x1 > render_width)
        x1 = render_width;
    if (x1 <= x0)
        return;
    if (mat->isSkyBox == 0)
    {
        int dTempBaDx = (triangle->b.y - triangle->c.y) << SHIFT_FACTOR;
        int dTempBbDx = (triangle->c.y - triangle->a.y) << SHIFT_FACTOR;
        int Ba, Bb, Bc;
        calc_bar_coords(triangle, &Ba, &Bb, &Bc, divider, x0, y);
        int stepBa = fixed_div(dTempBaDx, divider);
        int stepBb = fixed_div(dTempBbDx, divider);
        int32_t Lcur = L0 + (int32_t)(x0 - xStart) * dLdx;
        for (int x = x0; x < x1; x++)
        {
            uint16_t color = 0;
            if (mat->textureSize == 0)
                color = mat->diffuse;
            else
                color = texturing(triangle, mat, Ba, Bb, Bc);
            if (SHADING_ENABLED)
            {
                shading(&color, light, Lcur >> LIGHT_LERP_SHIFT);
                Lcur += dLdx;
            }
            // Upscale to output buffer (output_scale handles LCD scaling)
            for (uint8_t dy = 0; dy < output_scale; dy++)
                for (uint8_t dx = 0; dx < output_scale; dx++)
                    _painter->draw_pixel(x * output_scale + dx, y * output_scale + dy, color);
            // draw_pixel(x, y, color);
            Ba += stepBa;
            Bb += stepBb;
            Bc = SCALE_FACTOR - Ba - Bb;
        }
    }
    else
    {
        int dTempBaDx = (triangle->b.y - triangle->c.y) << SHIFT_FACTOR;
        int dTempBbDx = (triangle->c.y - triangle->a.y) << SHIFT_FACTOR;
        int Ba, Bb, Bc;
        calc_bar_coords(triangle, &Ba, &Bb, &Bc, divider, x0, y);
        int stepBa = fixed_div(dTempBaDx, divider);
        int stepBb = fixed_div(dTempBbDx, divider);
        for (int x = x0; x < x1; x++)
        {
            uint16_t color = 0;
            if (mat->textureSize == 0)
                color = mat->diffuse;
            else
                color = texturing(triangle, mat, Ba, Bb, Bc);
            for (uint8_t dy = 0; dy < output_scale; dy++)
                for (uint8_t dx = 0; dx < output_scale; dx++)
                    _painter->draw_pixel(x * output_scale + dx, y * output_scale + dy, color);
            // draw_pixel(x, y, color);
            Ba += stepBa;
            Bb += stepBb;
            Bc = SCALE_FACTOR - Ba - Bb;
        }
    }
}

inline void swap_int32(int32_t *x, int32_t *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}

void tri(TriangleToRender *triangle, Material *mat, int32_t lightDistances[], PointLight *light)
{
    int32_t x, y, z, uv, l, Lx;
    if (triangle->a.y > triangle->b.y)
    {
        swap_int32(&triangle->a.w, &triangle->b.w);
        swap_int32(&triangle->a.z, &triangle->b.z);
        swap_int32(&triangle->a.y, &triangle->b.y);
        swap_int32(&triangle->a.x, &triangle->b.x);

        swap_int32(&triangle->uvA.x, &triangle->uvB.x);
        swap_int32(&triangle->uvA.y, &triangle->uvB.y);

        swap_int32(&lightDistances[0], &lightDistances[1]);
    }
    if (triangle->a.y > triangle->c.y)
    {
        swap_int32(&triangle->a.w, &triangle->c.w);
        swap_int32(&triangle->a.z, &triangle->c.z);
        swap_int32(&triangle->a.y, &triangle->c.y);
        swap_int32(&triangle->a.x, &triangle->c.x);

        swap_int32(&triangle->uvA.x, &triangle->uvC.x);
        swap_int32(&triangle->uvA.y, &triangle->uvC.y);

        swap_int32(&lightDistances[0], &lightDistances[2]);
    }
    if (triangle->b.y > triangle->c.y)
    {
        swap_int32(&triangle->b.w, &triangle->c.w);
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
    int xx = x = triangle->a.x;
    int32_t Lxx = Lx = ((int32_t)lightDistances[0]) << LIGHT_LERP_SHIFT;

    int dx01 = triangle->b.x - triangle->a.x;
    int dy01 = triangle->b.y - triangle->a.y;

    int dx02 = triangle->c.x - triangle->a.x;
    int dy02 = triangle->c.y - triangle->a.y;

    int dx12 = triangle->c.x - triangle->b.x;
    int dy12 = triangle->c.y - triangle->b.y;

    int32_t dL01 = dy01 ? (((int32_t)(lightDistances[1] - lightDistances[0])) << LIGHT_LERP_SHIFT) / dy01 : 0;
    int32_t dL02 = dy02 ? (((int32_t)(lightDistances[2] - lightDistances[0])) << LIGHT_LERP_SHIFT) / dy02 : 0;
    int32_t dL12 = dy12 ? (((int32_t)(lightDistances[2] - lightDistances[1])) << LIGHT_LERP_SHIFT) / dy12 : 0;

    int q2 = 0;

    int xxd = 1 - ((dx02 < 0) << 1);

    int32_t divider = (triangle->b.y - triangle->c.y) * (triangle->a.x - triangle->c.x) + (triangle->c.x - triangle->b.x) * (triangle->a.y - triangle->c.y);
    divider <<= SHIFT_FACTOR;

    if (triangle->a.y < triangle->b.y)
    {
        int q = 0;
        int xd = 1 - ((dx01 < 0) << 1);
        while (y <= triangle->b.y)
        {
            rasterize(y, x, xx, triangle, mat, divider, light, Lx, Lxx);
            y += 1;
            q += dx01;
            q2 += dx02;
            Lx += dL01;
            Lxx += dL02;
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
        int q = 0;
        x = triangle->b.x;
        Lx = ((int32_t)lightDistances[1]) << LIGHT_LERP_SHIFT;
        int xd = 1 - ((dx12 < 0) << 1);

        while (y <= triangle->c.y && y < render_height)
        {
            rasterize(y, x, xx, triangle, mat, divider, light, Lx, Lxx);
            y += 1;
            q += dx12;
            q2 += dx02;
            Lx += dL12;
            Lxx += dL02;
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
        TriangleToRender triangle =
            {
                {
                    triScene->TriangleOnScreen.a.x,
                    triScene->TriangleOnScreen.a.y,
                    triScene->TriangleOnScreen.a.z,
                    inverse(triScene->TriangleOnScreen.a.z),
                },
                {
                    triScene->TriangleOnScreen.b.x,
                    triScene->TriangleOnScreen.b.y,
                    triScene->TriangleOnScreen.b.z,
                    inverse(triScene->TriangleOnScreen.b.z),
                },
                {
                    triScene->TriangleOnScreen.c.x,
                    triScene->TriangleOnScreen.c.y,
                    triScene->TriangleOnScreen.c.z,
                    inverse(triScene->TriangleOnScreen.c.z),
                },
                {
                    triScene->UV.a.x,
                    triScene->UV.a.y,
                },
                {
                    triScene->UV.b.x,
                    triScene->UV.b.y,
                },
                {
                    triScene->UV.c.x,
                    triScene->UV.c.y,
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
    for (int i = 0; i < mesh->transformationsNum; i++)
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
