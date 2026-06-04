#ifndef PUPPETCALC_h
#define PUPPETCALC_h

#include <stdint.h>
#include <stdlib.h>
#include "vectors.h"
#include "puppet.h"

void make_local_matrix(PuppetBone *PuppetBone);
void make_world_matrix(PuppetBone *PuppetBone, int *parentWorldMatrix);
void update_world_matrices(Puppet *puppet);

#endif