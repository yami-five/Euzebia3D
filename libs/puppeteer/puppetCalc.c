#include "puppetCalc.h"
#include <string.h>

void make_local_matrix(e3d_PuppetBone *e3d_PuppetBone) {
  int32_t angleIndex = radian_to_index(float_to_fixed(e3d_PuppetBone->angle));
  int16_t sin = fast_sin(angleIndex);
  int16_t cos = fast_cos(angleIndex);
  e3d_PuppetBone->localMatrix[0] = cos;
  e3d_PuppetBone->localMatrix[1] = -sin;
  e3d_PuppetBone->localMatrix[2] = e3d_PuppetBone->x << SHIFT_FACTOR;
  e3d_PuppetBone->localMatrix[3] = sin;
  e3d_PuppetBone->localMatrix[4] = cos;
  e3d_PuppetBone->localMatrix[5] = e3d_PuppetBone->y << SHIFT_FACTOR;
  e3d_PuppetBone->localMatrix[6] = 0;
  e3d_PuppetBone->localMatrix[7] = 0;
  e3d_PuppetBone->localMatrix[8] = SCALE_FACTOR;
}

void make_world_matrix(e3d_PuppetBone *e3d_PuppetBone, int *parentWorldMatrix) {
  int *result = mul_matrices(parentWorldMatrix, e3d_PuppetBone->localMatrix, 3, 3);
  for (uint8_t i = 0; i < 9; i++) {
    e3d_PuppetBone->worldMatrix[i] = result[i];
  }
  free(result);
}

void update_bones_world_matrices(e3d_PuppetBone *e3d_PuppetBone,
                                 int *parentWorldMatrix) {
  make_local_matrix(e3d_PuppetBone);
  make_world_matrix(e3d_PuppetBone, parentWorldMatrix);
  for (uint8_t i = 0; i < e3d_PuppetBone->childPuppetBonesNumLayer1; i++) {
    update_bones_world_matrices(&e3d_PuppetBone->childPuppetBonesLayer1[i],
                                e3d_PuppetBone->worldMatrix);
  }
  for (uint8_t i = 0; i < e3d_PuppetBone->childPuppetBonesNumLayer2; i++) {
    update_bones_world_matrices(&e3d_PuppetBone->childPuppetBonesLayer2[i],
                                e3d_PuppetBone->worldMatrix);
  }
}

void update_world_matrices(e3d_Puppet *puppet) {
  int32_t angleIndex = radian_to_index(float_to_fixed(puppet->angle));
  int16_t sin = fast_sin(angleIndex);
  int16_t cos = fast_cos(angleIndex);
  puppet->localMatrix[0] = cos;
  puppet->localMatrix[1] = -sin;
  puppet->localMatrix[2] = puppet->x << SHIFT_FACTOR;
  puppet->localMatrix[3] = sin;
  puppet->localMatrix[4] = cos;
  puppet->localMatrix[5] = puppet->y << SHIFT_FACTOR;
  puppet->localMatrix[6] = puppet->localMatrix[7] = 0;
  puppet->localMatrix[8] = SCALE_FACTOR;
  memcpy(puppet->worldMatrix, puppet->localMatrix, sizeof(puppet->localMatrix));
  for (uint8_t i = 0; i < puppet->puppetBonesNum; i++) {
    update_bones_world_matrices(&puppet->puppetBones[i], puppet->worldMatrix);
  }
}