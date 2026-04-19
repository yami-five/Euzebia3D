#include "mesh.h"

TransformInfo *add_transformation(TransformInfo *currentTransformations, uint32_t *currentTransformationsNum, float w, float x, float y, float z, ModelTransformType transformationType)
{
    if (transformationType > MODEL_TRANSFORM_SCALE)
        return currentTransformations;

    *currentTransformationsNum += 1;
    TransformInfo *newTransformations = (TransformInfo *)realloc(currentTransformations, *currentTransformationsNum * sizeof(TransformInfo));
    newTransformations[*currentTransformationsNum - 1].transformVector = (TransformVector *)malloc(sizeof(TransformVector));
    newTransformations[*currentTransformationsNum - 1].transformType = transformationType;
    newTransformations[*currentTransformationsNum - 1].transformVector->w = float_to_fixed(w);
    newTransformations[*currentTransformationsNum - 1].transformVector->x = float_to_fixed(x);
    newTransformations[*currentTransformationsNum - 1].transformVector->y = float_to_fixed(y);
    newTransformations[*currentTransformationsNum - 1].transformVector->z = float_to_fixed(z);

    return newTransformations;
}

void modify_mesh_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex)
{
    modify_transformation(currentTransformations, w, x, y, z, transformationIndex);
}

void free_model(Mesh *mesh)
{
    if (mesh == NULL)
        return;
    if (mesh->transformations != NULL)
    {
        for (uint32_t i = 0; i < mesh->transformationsNum; i++)
        {
            free(mesh->transformations[i].transformVector);
        }
        free(mesh->transformations);
    }
    free(mesh->mat);
    free(mesh->faces);
    free(mesh->vertices);
    free(mesh->textureCoords);
    free(mesh->uv);
    free(mesh->normals);
    free(mesh->vn);
    free(mesh);
}
