#include "common_types.slang"
#include "random.slang"

[shader("raygen")]
void raygenMain() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;
    float2 pixel = (float2(launchIndex) + 0.5) / float2(launchSize);
    float aspect = float(launchSize.x) / float(launchSize.y);
    float2 ndc;
    ndc.x = 2.0 * pixel.x - 1.0;
    ndc.y = -(2.0 * pixel.y - 1.0);

    uint seed = uint(pixel.x) * uint(pixel.y * ndc.y) * uint(pixel.x * ndc.x);
    uint state = Hash_Wang(seed);

    // カメラ基底ベクトル
    float3 forward = normalize(kTarget - CamPos.xyz);
    float3 right = normalize(cross(forward, kUp));
    float3 up = cross(right, forward);

    // FOVからスクリーン面の大きさを決定
    float t = tan(0.5 * kVFov);

    Payload payload;
    payload.miss_frag = false;
    payload.seed = seed;

    RayDesc rayDesc;

    int sampleCount = 16;
    float3 radiance = float3(0.0, 0.0, 0.0);

    for (int sy = 0; sy < sqrt(sampleCount); sy++) {
        for (int sx = 0; sx < sqrt(sampleCount); sx++) {
            float2 jitter = sampleDisk(state) * (1.0 / float2(launchSize));
            float2 ndcJ = float2(2.0 * (pixel.x + jitter.x) - 1.0, -2.0 * (pixel.y + jitter.y) + 1.0);
            float3 dir = normalize(forward +
                                   ndcJ.x * aspect * t * right +
                                   ndcJ.y * t * up);

            rayDesc.Origin = CamPos.xyz;
            rayDesc.Direction = dir;
            rayDesc.TMin = 0.001;
            rayDesc.TMax = 1e6;

            payload.radiance = float3(0.0, 0.0, 0.0);
            payload.normal = float3(0.0, 0.0, 0.0);
            payload.depth = 0;

            TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, payload);
            radiance += payload.radiance;
        }
    }
    radiance /= sampleCount;

    outputTexture[launchIndex] = float4(radiance, 1.0);
    return;
}