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
static uint32_t zBufferSize = 0;
static int32_t *zBuffer = NULL;
#define MAX_ZBUFFER_BYTES 200000 
#define SHADING_ENABLED 1
// Render can be downscaled: render_scale=2 -> 160x120 rendered, scaled to LCD in painter.

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

void clear_zbuffuer();
static void configure_render_dimensions(void)
{
    // Adjust render dims and (re)allocate z-buffer when scale changes.
    if (render_scale == 0)
        render_scale = 2;
    output_scale = render_scale;
    render_width = BASE_WIDTH / render_scale;
    render_height = BASE_HEIGHT / render_scale;
    render_width_half = render_width >> 1;
    render_height_half = render_height >> 1;

    uint32_t newSize = render_width * render_height;
    uint32_t neededBytes = newSize * sizeof(int32_t);
    if (neededBytes > MAX_ZBUFFER_BYTES)
    {
        render_scale = 2;
        output_scale = render_scale;
        render_width = BASE_WIDTH / render_scale;
        render_height = BASE_HEIGHT / render_scale;
        render_width_half = render_width >> 1;
        render_height_half = render_height >> 1;
        newSize = render_width * render_height;
        neededBytes = newSize * sizeof(int32_t);
    }

    if (zBufferSize != newSize)
    {
        free(zBuffer);
        zBuffer = (int32_t *)malloc(neededBytes);
        if (zBuffer != NULL)
            zBufferSize = newSize;
        else
            zBufferSize = 0;
    }
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
    clear_zbuffuer();
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
        Quaternion *result = mul_quaternion(&q, &q_vertex);
        if (result == NULL || result->vec == NULL)
        {
            if (result != NULL)
            {
                free(result->vec);
                free(result);
            }
            return;
        }
        Quaternion *result2 = mul_quaternion(result, &qInv);
        if (result2 == NULL || result2->vec == NULL)
        {
            free(result->vec);
            free(result);
            if (result2 != NULL)
            {
                free(result2->vec);
                free(result2);
            }
            return;
        }
        vertices[i * 3] = result2->vec->x;
        vertices[i * 3 + 1] = result2->vec->y;
        vertices[i * 3 + 2] = result2->vec->z;
        free(result->vec);
        free(result);
        free(result2->vec);
        free(result2);
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

void shading(uint16_t *color, int32_t lightDistances[], PointLight *light, int Ba, int Bb, int Bc)
{
    int32_t lightDistance = (fixed_mul(Ba, lightDistances[0]) + fixed_mul(Bb, lightDistances[1]) + fixed_mul(Bc, lightDistances[2]));
    // if (lightDistance <= 0)
    // {
    //     *color = 0;
    //     return;
    // }
    // Clamp minimum light to keep pixels from going fully dark on edges
    const int32_t AMBIENT_MIN = SCALE_FACTOR / 20;
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

uint16_t texturing(Triangle2D *triangle, Material *mat, int Ba, int Bb, int Bc)
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

void calc_bar_coords(Triangle2D *triangle, int *Ba, int *Bb, int *Bc, int32_t divider, int x, int y)
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

void clear_zbuffuer()
{
    if (zBuffer == NULL)
        return;
    for (uint32_t i = 0; i < zBufferSize; i++)
        zBuffer[i] = INT32_MAX;
}

uint8_t check_zbuffer(int x, int y, int z)
{
    if (zBuffer == NULL)
        return 0;
    uint32_t addr = x * render_height + y;
    if (z < zBuffer[addr])
    {
        zBuffer[addr] = z;
        return 1;
    }
    else
        return 0;
}

void rasterize(int y, int x0, int x1, Triangle2D *triangle, Material *mat, int32_t lightDistances[], int32_t divider, PointLight *light)
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
    }
    x1 += 1;
    if (x1 < 0 || x0 >= render_width)
        return;
    if (x0 < 0)
        x0 = 0;
    if (x1 > render_width)
        x1 = render_width;
    if (mat->isSkyBox == 0)
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
            int z = calc_pixel_depth(Ba, Bb, Bc, triangle->a.z, triangle->b.z, triangle->c.z);
            if (check_zbuffer(x, y, z))
            {
                if (mat->textureSize == 0)
                    color = mat->diffuse;
                else
                    color = texturing(triangle, mat, Ba, Bb, Bc);
                if (SHADING_ENABLED)
                    shading(&color, lightDistances, light, Ba, Bb, Bc);
                // Upscale to output buffer (output_scale handles LCD scaling)
                for (uint8_t dy = 0; dy < output_scale; dy++)
                    for (uint8_t dx = 0; dx < output_scale; dx++)
                        _painter->draw_pixel(x * output_scale + dx, y * output_scale + dy, color);
                // draw_pixel(x, y, color);
            }
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

void tri(Triangle2D *triangle, Material *mat, int32_t lightDistances[], PointLight *light)
{
    int32_t x, y, z, uv, l;
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
    int xx = x = triangle->a.x;

    int dx01 = triangle->b.x - triangle->a.x;
    int dy01 = triangle->b.y - triangle->a.y;

    int dx02 = triangle->c.x - triangle->a.x;
    int dy02 = triangle->c.y - triangle->a.y;

    int dx12 = triangle->c.x - triangle->b.x;
    int dy12 = triangle->c.y - triangle->b.y;

    int q2 = 0;
    int xxd = 1;

    if (triangle->c.x < triangle->a.x)
        xxd = -1;

    int32_t divider = (triangle->b.y - triangle->c.y) * (triangle->a.x - triangle->c.x) + (triangle->c.x - triangle->b.x) * (triangle->a.y - triangle->c.y);
    divider <<= SHIFT_FACTOR;

    if (triangle->a.y < triangle->b.y)
    {
        int q = 0;
        int xd = 1;
        if (triangle->b.x < triangle->a.x)
            xd = -1;
        while (y <= triangle->b.y)
        {
            rasterize(y, x, xx, triangle, mat, lightDistances, divider, light);
            y += 1;
            q += dx01;
            q2 += dx02;
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
        int x = triangle->b.x;
        int xd = 1;
        if (triangle->c.x < triangle->b.x)
            xd = -1;

        while (y <= triangle->c.y && y < render_height)
            {
                rasterize(y, x, xx, triangle, mat, lightDistances, divider, light);
            y += 1;
            q += dx12;
            q2 += dx02;
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

void draw_model(Mesh *mesh, PointLight *pLight, Camera *camera)
{
    uint16_t verticesCounter = mesh->verticesCounter;
    uint16_t vnCounter = mesh->vnCounter;
    int32_t *verticesModified = (int32_t *)malloc(sizeof(int32_t) * verticesCounter * 3);
    int32_t *verticesOnScreen = (int32_t *)malloc(sizeof(int32_t) * verticesCounter * 3);
    int32_t *normalsModified = (int32_t *)malloc(sizeof(int32_t) * vnCounter * 3);
    uint8_t *vertexValid = (uint8_t *)malloc(sizeof(uint8_t) * verticesCounter);

    if (verticesModified == NULL || verticesOnScreen == NULL || normalsModified == NULL || vertexValid == NULL)
    {
        free(verticesModified);
        free(verticesOnScreen);
        free(normalsModified);
        free(vertexValid);
        return;
    }

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
        if (w == 0 || z <= 0)
        {
            vertexValid[i / 3] = 0;
            continue;
        }
        verticesOnScreen[i] = fixed_div(x, w) + render_width_half;
        verticesOnScreen[i + 1] = fixed_div(y, w) + render_height_half;
        verticesOnScreen[i + 2] = z;
        vertexValid[i / 3] = 1;
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
        if (vertexValid[a] == 0 || vertexValid[b] == 0 || vertexValid[c] == 0)
            continue;
        uint16_t uvA = mesh->uv[i];
        uint16_t uvB = mesh->uv[i + 1];
        uint16_t uvC = mesh->uv[i + 2];
        uint16_t normalA = mesh->normals[i];
        uint16_t normalB = mesh->normals[i + 1];
        uint16_t normalC = mesh->normals[i + 2];
        Triangle2D triangle =
            {
                {verticesOnScreen[a * 3],
                 verticesOnScreen[a * 3 + 1],
                 inverse(verticesOnScreen[a * 3 + 2])},
                {verticesOnScreen[b * 3],
                 verticesOnScreen[b * 3 + 1],
                 inverse(verticesOnScreen[b * 3 + 2])},
                {verticesOnScreen[c * 3],
                 verticesOnScreen[c * 3 + 1],
                 inverse(verticesOnScreen[c * 3 + 2])},
                {mesh->textureCoords[uvA * 2],
                 mesh->textureCoords[uvA * 2 + 1]},
                {mesh->textureCoords[uvB * 2],
                 mesh->textureCoords[uvB * 2 + 1]},
                {mesh->textureCoords[uvC * 2],
                 mesh->textureCoords[uvC * 2 + 1]}};
        if (!check_if_triangle_visible(&triangle))
            continue;
        // normal vector
        int32_t lightDistances[3] = {0, 0, 0};
#ifdef SHADING_ENABLED
        if (mesh->mat->isSkyBox == 0)
        {
            normalVectorA.x = normalsModified[normalA * 3];
            normalVectorA.y = normalsModified[normalA * 3 + 1];
            normalVectorA.z = normalsModified[normalA * 3 + 2];
            normalVectorB.x = normalsModified[normalB * 3];
            normalVectorB.y = normalsModified[normalB * 3 + 1];
            normalVectorB.z = normalsModified[normalB * 3 + 2];
            normalVectorC.x = normalsModified[normalC * 3];
            normalVectorC.y = normalsModified[normalC * 3 + 1];
            normalVectorC.z = normalsModified[normalC * 3 + 2];

            if (!norm_vector_safe(&normalVectorA) || !norm_vector_safe(&normalVectorB) || !norm_vector_safe(&normalVectorC))
                continue;

            // light direction
            Triangle3D triangle3D = {
                {verticesModified[a * 3],
                 verticesModified[a * 3 + 1],
                 verticesModified[a * 3 + 2]},
                {verticesModified[b * 3],
                 verticesModified[b * 3 + 1],
                 verticesModified[b * 3 + 2]},
                {verticesModified[c * 3],
                 verticesModified[c * 3 + 1],
                 verticesModified[c * 3 + 2]}};

            lightDirectionA.x = pLight->position.x - triangle3D.a.x;
            lightDirectionA.y = pLight->position.y - triangle3D.a.y;
            lightDirectionA.z = pLight->position.z - triangle3D.a.z;
            lightDirectionB.x = pLight->position.x - triangle3D.b.x;
            lightDirectionB.y = pLight->position.y - triangle3D.b.y;
            lightDirectionB.z = pLight->position.z - triangle3D.b.z;
            lightDirectionC.x = pLight->position.x - triangle3D.c.x;
            lightDirectionC.y = pLight->position.y - triangle3D.c.y;
            lightDirectionC.z = pLight->position.z - triangle3D.c.z;

            if (!norm_vector_safe(&lightDirectionA) || !norm_vector_safe(&lightDirectionB) || !norm_vector_safe(&lightDirectionC))
                continue;

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
#endif
        tri(&triangle, mesh->mat, lightDistances, pLight);
    }

    free(verticesModified);
    free(verticesOnScreen);
    free(normalsModified);
    free(vertexValid);
}

static IRenderer renderer = {
    .init_renderer = init_renderer,
    .draw_model = draw_model,
    .clear_zbuffer = clear_zbuffuer,
    .set_scale = renderer_set_scale};

const IRenderer *get_renderer(void)
{
    return &renderer;
}
