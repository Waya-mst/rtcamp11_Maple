import common_types;
#include "common_bindings.slang"

[shader("closesthit")]
void closestHitMain(in BuiltInTriangleIntersectionAttributes attr, inout RadiancePayload payload)
{
    const int prim = PrimitiveIndex();
    const uint i0 = indices[prim * 3 + 0];
    const uint i1 = indices[prim * 3 + 1];
    const uint i2 = indices[prim * 3 + 2];

    float w0 = 1.0 - attr.barycentrics.x - attr.barycentrics.y;
    float3 N = normalize(vertices[i0].normal.xyz * w0 +
                         vertices[i1].normal.xyz * attr.barycentrics.x +
                         vertices[i2].normal.xyz * attr.barycentrics.y);

    float2 uv = vertices[i0].texCoord.xy * w0 +
                vertices[i1].texCoord.xy * attr.barycentrics.x +
                vertices[i2].texCoord.xy * attr.barycentrics.y;

    float3 albedo = BaseColorTex.Sample(BaseSampler, uv, 0).rgb;

    // 超シンプル Lambert
    float NdotL = max(0.0, dot(N, normalize(-SunDir.xyz)));
    float3 color = albedo * (0.1 + 0.9 * NdotL);

    // payload.radiance = 0.5 * N + 0.5;
    payload.radiance = albedo;
}