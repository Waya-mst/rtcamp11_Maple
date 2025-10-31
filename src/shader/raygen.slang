import common_types;
#include "common_bindings.slang"

[shader("raygen")]
void raygenMain() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;
    float2 pixel = (float2(launchIndex) + 0.5) / float2(launchSize);
    float aspect = float(launchSize.x) / float(launchSize.y);
    float2 ndc;
    ndc.x = 2.0 * pixel.x - 1.0;
    ndc.y = -(2.0 * pixel.y - 1.0);

    // カメラ基底ベクトル
    float3 forward = normalize(kTarget - CamPos.xyz);
    float3 right = normalize(cross(forward, kUp));
    float3 up = cross(right, forward);

    // FOVからスクリーン面の大きさを決定
    float t = tan(0.5 * kVFov);
    float3 dir = normalize(forward
                          + ndc.x * aspect * t * right
                          + ndc.y * t * up);

    RadiancePayload payload;

    RayDesc rayDesc;
    rayDesc.Origin = CamPos.xyz;
    rayDesc.Direction = dir;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 1e6;

    TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, payload);

    float3 color = payload.radiance;

    outputTexture[launchIndex] = float4(color, 1.0);
}