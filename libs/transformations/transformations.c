#include "transformations.h"

void modify_transformation(TransformInfo *currentTransformations, float w, float x, float y, float z, uint32_t transformationIndex)
{
    currentTransformations[transformationIndex].transformVector->w = float_to_fixed(w);
    currentTransformations[transformationIndex].transformVector->x = float_to_fixed(x);
    currentTransformations[transformationIndex].transformVector->y = float_to_fixed(y);
    currentTransformations[transformationIndex].transformVector->z = float_to_fixed(z);
}
