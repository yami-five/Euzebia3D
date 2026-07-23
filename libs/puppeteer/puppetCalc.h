#ifndef PUPPETCALC_h
#define PUPPETCALC_h

#include "puppet.h"
#include "vectors.h"
#include <stdint.h>
#include <stdlib.h>

void make_local_matrix(e3d_PuppetBone *e3d_PuppetBone);
void make_world_matrix(e3d_PuppetBone *e3d_PuppetBone, int *parentWorldMatrix);
void update_world_matrices(e3d_Puppet *puppet);

#endif