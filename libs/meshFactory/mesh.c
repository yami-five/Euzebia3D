#include "mesh.h"

e3d_TransformInfo *add_transformation(e3d_TransformInfo *currentTransformations,
                                  uint32_t *currentTransformationsNum, float w,
                                  float x, float y, float z,
                                  e3d_ModelTransformType transformationType) {
  if (transformationType > MODEL_TRANSFORM_SCALE)
    return currentTransformations;

  uint32_t oldTransformationsNum = *currentTransformationsNum;
  uint32_t newTransformationsNum = oldTransformationsNum + 1u;
  e3d_TransformInfo *newTransformations = (e3d_TransformInfo *)realloc(
      currentTransformations, newTransformationsNum * sizeof(e3d_TransformInfo));
  if (newTransformations == NULL)
    return currentTransformations;

  e3d_TransformVector *newVector =
      (e3d_TransformVector *)malloc(sizeof(e3d_TransformVector));
  if (newVector == NULL)
    return newTransformations;

  newTransformations[oldTransformationsNum].transformVector = newVector;
  newTransformations[oldTransformationsNum].transformType = transformationType;
  newTransformations[oldTransformationsNum].transformVector->w =
      float_to_fixed(w);
  newTransformations[oldTransformationsNum].transformVector->x =
      float_to_fixed(x);
  newTransformations[oldTransformationsNum].transformVector->y =
      float_to_fixed(y);
  newTransformations[oldTransformationsNum].transformVector->z =
      float_to_fixed(z);
  *currentTransformationsNum = newTransformationsNum;

  return newTransformations;
}

void modify_mesh_transformation(e3d_TransformInfo *currentTransformations, float w,
                                float x, float y, float z,
                                uint32_t transformationIndex) {
  modify_transformation(currentTransformations, w, x, y, z,
                        transformationIndex);
}

void free_model(e3d_Mesh *mesh) {
  if (mesh == NULL)
    return;
  if (mesh->transformations != NULL) {
    for (uint32_t i = 0; i < mesh->transformationsNum; i++) {
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
