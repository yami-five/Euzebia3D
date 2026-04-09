#include "IMeshFactory.h"
#include "IStorage.h"
#include "meshFactory.h"
#include "mesh.h"
#include "fpa.h"
#include <stdlib.h>

static volatile const IStorage *_storage = NULL;

void init_mesh_factory(volatile const IStorage *storage)
{
    _storage=storage;
}


Mesh *createMesh(Material *mat, uint8_t meshIndex)
{
    const Model *obj = _storage->get_model(meshIndex);
    Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
    mesh->verticesCounter = obj->verticesCounter;
    mesh->facesCounter = obj->facesCounter;
    mesh->vnCounter = obj->vnCounter;
    mesh->vertices = (int32_t *)malloc(sizeof(int32_t) * obj->verticesCounter * 3);
    mesh->faces = (uint16_t *)malloc(sizeof(uint16_t) * obj->facesCounter * 3);
    mesh->textureCoords = (int32_t *)malloc(sizeof(int32_t) * obj->textureCoordsCounter * 2);
    mesh->uv = (uint16_t *)malloc(sizeof(uint16_t) * obj->facesCounter * 3);
    mesh->vn = (int32_t *)malloc(sizeof(int32_t) * obj->vnCounter * 3);
    mesh->normals = (uint16_t *)malloc(sizeof(uint16_t) * obj->facesCounter * 3);
    mesh->mat = mat;
    mesh->transformations = NULL;
    mesh->transformationsNum = 0;

    for (uint16_t i = 0; i < obj->facesCounter * 3; i++)
    {
        mesh->faces[i] = obj->faces[i];
        mesh->uv[i] = obj->uv[i];
        mesh->normals[i] = obj->normals[i];
    }

    for (uint16_t i = 0; i < obj->verticesCounter * 3; i++)
    {
        mesh->vertices[i] = float_to_fixed(obj->vertices[i]);
    }

    for (uint16_t i = 0; i < obj->vnCounter * 3; i++)
    {
        mesh->vn[i] = float_to_fixed(obj->vn[i]);
    }

    for (uint16_t i = 0; i < obj->textureCoordsCounter * 2; i++)
    {
        mesh->textureCoords[i] = float_to_fixed(obj->textureCoords[i]);
    }

    // free((void *)obj);
    return mesh;
}

Mesh *create_colored_mesh(uint16_t color, uint8_t meshIndex)
{
    Material *material = (Material *)malloc(sizeof(Material));
    material->diffuse = color;
    material->texture = 0;
    material->textureSize = 0;
    material->isSkyBox = 0;
    return createMesh(material, meshIndex);
}

Mesh *create_textured_mesh(uint8_t imageIndex, uint8_t meshIndex)
{
    Material *material = (Material *)malloc(sizeof(Material));
    material->diffuse = 0;
    material->texture = _storage->get_image(imageIndex)->image;
    material->textureSize = _storage->get_image(imageIndex)->heigth;
    material->isSkyBox = 0;
    return createMesh(material, meshIndex);
}

Mesh *create_colored_skybox(uint16_t color)
{
    Material *material = (Material *)malloc(sizeof(Material));
    material->diffuse = color;
    material->texture = 0;
    material->textureSize = 0;
    material->isSkyBox = 1;
    return createMesh(material, 0);
}

Mesh *create_textured_skybox(uint8_t imageIndex)
{
    Material *material = (Material *)malloc(sizeof(Material));
    material->diffuse = 0;
    material->texture = _storage->get_image(imageIndex)->image;
    material->textureSize = _storage->get_image(imageIndex)->heigth;
    material->isSkyBox = 1;
    return createMesh(material, 0);
}

static IMeshFactory renderer = {
    .init_mesh_factory = init_mesh_factory,
    .create_colored_mesh = create_colored_mesh,
    .create_textured_mesh = create_textured_mesh,
    .create_colored_skybox = create_colored_skybox,
    .create_textured_skybox = create_textured_skybox,
};

const IMeshFactory *get_meshFactory(void)
{
    return &renderer;
}