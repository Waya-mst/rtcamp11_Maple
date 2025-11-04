import common_types;
#include "common_bindings.slang"

[shader("closesthit")]
void closestHitMain(in BuiltInTriangleIntersectionAttributes attr, inout RadiancePayload payload)
{
    const int prim = PrimitiveIndex();
    const uint i0 = indices[prim * 3 + 0];
    const uint i1 = indices[prim * 3 + 1];
    const uint i2 = indices[prim * 3 + 2];

    float u = attr.barycentrics.x;
    float v = attr.barycentrics.y;
    float w = 1.0 - u - v;

    float3 N = normalize(vertices[i0].normal.xyz * w +
                         vertices[i1].normal.xyz * u +
                         vertices[i2].normal.xyz * v);

    float2 uv = vertices[i0].texCoord.xy * w +
                vertices[i1].texCoord.xy * u +
                vertices[i2].texCoord.xy * v;

    float3 pObj = vertices[i0].pos.xyz * w +
                  vertices[i1].pos.xyz * u +
                  vertices[i2].pos.xyz * v;

    float3 albedo = BaseColorTex.SampleLevel(BaseSampler, uv, 0).rgb;

    float3 hitPos = mul(float4(pObj, 1.0), ObjectToWorld4x3()).xyz;
    float3 inRay = -WorldRayDirection();
    float3 Ns = normalize(mul(N, (float3x3)ObjectToWorld3x4()));
    float3 Reflect = reflect(inRay, Ns);
    float eps = max(1e-4, 1e-3 * length(hitPos));

    RayDesc shadowRayDesc;
    shadowRayDesc.Origin = hitPos + Reflect * eps;
    shadowRayDesc.Direction = Reflect;
    shadowRayDesc.TMin = 0.001;
    shadowRayDesc.TMax = 1e6;
    ShadowPayload vis;
    vis.occluded = true;

    TraceRay(topLevelAS,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF, 1, 0, 1, shadowRayDesc, vis);

    // 超シンプル Lambert
    // float NdotL = max(0.0, dot(N, normalize(-SunDir.xyz)));
    // float3 color = albedo * (0.1 + 0.9 * NdotL);
    float3 color = albedo;

    if (!vis.occluded) {
        color = float3(1.0, 0.0, 1.0);
    }
    

    //payload.radiance = 0.5 * N + 0.5;
    payload.radiance = color;
}