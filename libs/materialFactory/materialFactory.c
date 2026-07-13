
#include "IMaterialFactory.h"
#include "IStorage.h"
#include "materialFactory.h"
#include "material.h"
#include "fpa.h"
#include <stdlib.h>

static const IStorage *_storage = NULL;

void init_material_factory(const IStorage *storage)
{
    _storage = storage;
}

Material *create_diffuse_mat(uint16_t color, float roughness, float metallic)
{
    if (roughness < 0.0f)
        roughness = 0.0f;
    else if (roughness > 1.0f)
        roughness = 1.0f;
    if (metallic < 0.0f)
        metallic = 0.0f;
    else if (metallic > 1.0f)
        metallic = 1.0f;

    Material *mat = (Material *)calloc(1, sizeof(Material));
    if (mat == NULL)
    {
        return NULL;
    }

    mat->diffuse = color;
    mat->metallic = (uint8_t)(metallic * 255.0f);
    mat->roughness = (uint8_t)(roughness * 255.0f);
    mat->transparent = false;
    mat->texture = NULL;
    mat->textureHeight = 0;
    mat->textureWidth = 0;
    mat->textureSize = 0;

    return mat;
}

Material *create_textured_mat(uint8_t imageIndex, float roughness, float metallic, bool transparent)
{
    if (roughness < 0.0f)
        roughness = 0.0f;
    else if (roughness > 1.0f)
        roughness = 1.0f;
    if (metallic < 0.0f)
        metallic = 0.0f;
    else if (metallic > 1.0f)
        metallic = 1.0f;

    Material *mat = (Material *)calloc(1, sizeof(Material));
    if (mat == NULL)
    {
        return NULL;
    }

    if (_storage == NULL)
    {
        free(mat);
        return NULL;
    }

    const Image *texture_temp = _storage->get_image(imageIndex);
    if (texture_temp == NULL)
    {
        free(mat);
        return NULL;
    }

    mat->diffuse = 0;
    mat->metallic = (uint8_t)(metallic * 255.0f);
    mat->roughness = (uint8_t)(roughness * 255.0f);
    mat->transparent = transparent;
    mat->texture = texture_temp->image;
    mat->textureHeight = texture_temp->heigth;
    mat->textureWidth = texture_temp->width;
    mat->textureSize = texture_temp->size;

    return mat;
}

static IMaterialFactory materialFactory = {
    .init_material_factory = init_material_factory,
    .create_textured_mat = create_textured_mat,
    .create_diffuse_mat = create_diffuse_mat,
};

const IMaterialFactory *get_materialFactory(void)
{
    return &materialFactory;
}
