import common_types;
#include "common_bindings.slang"

[shader("anyhit")]
void anyhitShadow(in BuiltInTriangleIntersectionAttributes attr, inout ShadowPayload vis)
{
    vis.occluded = true;
    AcceptHitAndEndSearch();
}