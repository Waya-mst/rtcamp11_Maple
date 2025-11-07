#include "common_types.slang"

[shader("miss")]
void missShadow(inout ShadowPayload vis)
{
    vis.occluded = false;
}