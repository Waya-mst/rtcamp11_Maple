#include "common_types.slang"

[shader("anyhit")]
void anyhitShadow(
    in BuiltInTriangleIntersectionAttributes attr,
    inout ShadowPayload vis
){
    vis.occluded = true;
    AcceptHitAndEndSearch();
}