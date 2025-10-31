import common_types;
#include "common_bindings.slang"

[shader("miss")]
void missShadow(inout ShadowPayload vis)
{
    vis.occluded = false;
}