import common_types;
#include "common_bindings.slang"

[shader("miss")]
void missMain(inout RadiancePayload payload)
{
    payload.radiance = SunColor.xyz;
}