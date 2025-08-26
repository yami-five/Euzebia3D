#include "IRenderer.h"
#include "renderer.h"

static volatile const IHardware *_hardware = NULL;
static volatile const IPainter *_painter = NULL;

static const uint8_t SCALE = 2;
static const uint8_t FOCAL_LENGTH = 90;
static const uint16_t WIDTH_DISPLAY = 320 >> (SCALE - 1);
static const uint16_t HEIGHT_DISPLAY = 240 >> (SCALE - 1);
static const uint16_t WIDTH_DOUBLED = 640 >> (SCALE - 1);
static const uint16_t HEIGHT_DOUBLED = 480 >> (SCALE - 1);
static const uint32_t ARRAY_SIZE = 153600;
static const uint16_t WIDTH_HALF = 160 >> (SCALE - 1);
static const uint16_t HEIGHT_HALF = 120 >> (SCALE - 1);
static const uint32_t FIRE_FLOOR_ADR = 76480;
static const uint32_t FIXED_FOCAL_LENGTH = 90 << SHIFT_FACTOR;
static const uint32_t TRIANGLE_CENTER_DIVIDER = 3 << SHIFT_FACTOR;
static const uint16_t ZBUFFERSIZE = 19200; // 160*120
static int zBuffer[19200];
#define SHADING_ENABLED 1

void clear_zbuffuer();

void init_renderer(volatile const IHardware *hardware, volatile const IPainter *painter)
{
    _hardware = hardware;
    _painter = painter;
    clear_zbuffuer();
    // init_sin_cos();
}

void triangle_center(Triangle3D *triangle, int32_t *center)
{
    center[0] = fixed_div((triangle->a.x + triangle->b.x + triangle->c.x), TRIANGLE_CENTER_DIVIDER);
    center[1] = fixed_div((triangle->a.y + triangle->b.y + triangle->c.y), TRIANGLE_CENTER_DIVIDER);
    center[2] = fixed_div((triangle->a.z + triangle->b.z + triangle->c.z), TRIANGLE_CENTER_DIVIDER);
}

void rotate(int *vertices, uint16_t verticesCounter, TransformVector *vector)
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
        Quaternion *result2 = mul_quaternion(result, &qInv);
        vertices[i * 3] = result2->vec->x;
        vertices[i * 3 + 1] = result2->vec->y;
        vertices[i * 3 + 2] = result2->vec->z;
        free(result->vec);
        free(result);
        free(result2->vec);
        free(result2);
    }
}

void translate(int *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i * 3] += vector->x;
        vertices[i * 3 + 1] += vector->y;
        vertices[i * 3 + 2] += vector->z;
    }
}

void scale(int *vertices, uint16_t verticesCounter, TransformVector *vector)
{
    for (uint16_t i = 0; i < verticesCounter; i++)
    {
        vertices[i * 3] = fixed_mul(vertices[i * 3], vector->x);
        vertices[i * 3 + 1] = fixed_mul(vertices[i * 3 + 1], vector->y);
        vertices[i * 3 + 2] = fixed_mul(vertices[i * 3 + 2], vector->z);
    }
}

void transform(int *vertices, uint16_t verticesCounter, TransformInfo *transformInfo)
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
    if (lightDistance < 50)
        lightDistance = 50;
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

    uint32_t lightFactor = fixed_mul(lightDistance, light->intensity);

    uint8_t r = (uint8_t)(fixed_mul(fixedR, lightFactor) >> SHIFT_FACTOR);
    uint8_t g = (uint8_t)(fixed_mul(fixedG, lightFactor) >> SHIFT_FACTOR);
    uint8_t b = (uint8_t)(fixed_mul(fixedB, lightFactor) >> SHIFT_FACTOR);

    *color = (r << 11) | (g << 5) | b;
}

uint16_t texturing(Triangle2D *triangle, Material *mat, int Ba, int Bb, int Bc)
{
    int uv_x = (fixed_mul(Ba, triangle->uvA.x) + fixed_mul(Bb, triangle->uvB.x) + fixed_mul(Bc, triangle->uvC.x)) * mat->textureSize;
    int uv_y = (fixed_mul(Ba, triangle->uvA.y) + fixed_mul(Bb, triangle->uvB.y) + fixed_mul(Bc, triangle->uvC.y)) * mat->textureSize;
    uv_x = uv_x >> SHIFT_FACTOR;
    uv_y = uv_y >> SHIFT_FACTOR;
    if (uv_x < 0)
        uv_x = 0;
    if (uv_x > mat->textureSize)
        uv_x = mat->textureSize - 1;
    if (uv_y < 0)
        uv_y = 0;
    if (uv_y > mat->textureSize)
        uv_y = mat->textureSize - 1;
    return mat->texture[uv_y * mat->textureSize + uv_x];
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
    // memset(zBuffer, UINT16_MAX, sizeof(zBuffer));
    for (uint16_t i = 0; i < ZBUFFERSIZE; i++)
        zBuffer[i] = INT32_MAX;
}

uint8_t check_zbuffer(int x, int y, int z)
{
    uint16_t addr = x * HEIGHT_DISPLAY + y;
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
    if (y < 0 || y >= HEIGHT_DISPLAY)
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
    if (x1 < 0 || x0 >= WIDTH_DISPLAY)
        return;
    if (x0 < 0)
        x0 = 0;
    if (x1 > WIDTH_DISPLAY)
        x1 = WIDTH_DISPLAY;
    if (mat->isSkyBox == 0)
    {
        for (int x = x0; x < x1; x++)
        {
            uint16_t color = 0;
            int Ba, Bb, Bc;
            calc_bar_coords(triangle, &Ba, &Bb, &Bc, divider, x, y);
            int z = calc_pixel_depth(Ba, Bb, Bc, triangle->a.z, triangle->b.z, triangle->c.z);
            if (check_zbuffer(x, y, z))
            {
                if (mat->textureSize == 0)
                    color = mat->diffuse;
                else
                    color = texturing(triangle, mat, Ba, Bb, Bc);
                if (SHADING_ENABLED)
                    shading(&color, lightDistances, light, Ba, Bb, Bc);
                _painter->draw_pixel(x * 2, y * 2, color);
                _painter->draw_pixel(x * 2 + 1, y * 2, color);
                _painter->draw_pixel(x * 2, y * 2 + 1, color);
                _painter->draw_pixel(x * 2 + 1, y * 2 + 1, color);
                // draw_pixel(x, y, color);
            }
        }
    }
    else
    {
        for (int x = x0; x < x1; x++)
        {
            uint16_t color = 0;
            int Ba, Bb, Bc;
            calc_bar_coords(triangle, &Ba, &Bb, &Bc, divider, x, y);
            if (mat->textureSize == 0)
                color = mat->diffuse;
            else
                color = texturing(triangle, mat, Ba, Bb, Bc);
            _painter->draw_pixel(x * 2, y * 2, color);
            _painter->draw_pixel(x * 2 + 1, y * 2, color);
            _painter->draw_pixel(x * 2, y * 2 + 1, color);
            _painter->draw_pixel(x * 2 + 1, y * 2 + 1, color);
            // draw_pixel(x, y, color);
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
    if (triangle->c.y < 0 || triangle->a.y > HEIGHT_DISPLAY)
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

        while (y <= triangle->c.y && y < HEIGHT_DISPLAY)
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
    int verticesModified[verticesCounter * 3];
    int verticesOnScreen[verticesCounter * 3];
    int normalsModified[vnCounter * 3];

    memcpy(verticesModified, mesh->vertices, verticesCounter * 3 * sizeof(int));
    memcpy(normalsModified, mesh->vn, vnCounter * 3 * sizeof(int));
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
        verticesOnScreen[i] = fixed_div(x, w) + WIDTH_HALF;
        verticesOnScreen[i + 1] = fixed_div(y, w) + HEIGHT_HALF;
        verticesOnScreen[i + 2] = z;
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

            norm_vector(&normalVectorA);
            norm_vector(&normalVectorB);
            norm_vector(&normalVectorC);

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

            norm_vector(&lightDirectionA);
            norm_vector(&lightDirectionB);
            norm_vector(&lightDirectionC);

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
}

static IRenderer renderer = {
    .init_renderer = init_renderer,
    .draw_model = draw_model,
    .clear_zbuffer = clear_zbuffuer};

const IRenderer *get_renderer(void)
{
    return &renderer;
}