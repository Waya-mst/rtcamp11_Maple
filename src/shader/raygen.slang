#include "common_types.slang"
#include "random.slang"
#include "util.slang"

[shader("raygen")]
void raygenMain() {
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;
    float2 pixel = (float2(launchIndex) + 0.5) / float2(launchSize);
    float aspect = float(launchSize.x) / float(launchSize.y);
    float2 ndc;
    ndc.x = 2.0 * pixel.x - 1.0;
    ndc.y = -(2.0 * pixel.y - 1.0);

    uint seed = launchIndex.x + launchIndex.y * launchSize.x;
    uint state = Hash_Wang(seed);

    // カメラ基底ベクトル
    float3 forward = normalize(kTarget - CamPos.xyz);
    float3 right = normalize(cross(forward, kUp));
    float3 up = cross(right, forward);

    // FOVからスクリーン面の大きさを決定
    float t = tan(0.5 * kVFov);

    float3 baseColor = float3(0.0, 0.0, 0.0);
    float transmission = 1.0;
    float ior = 0.0;
    float metallic = 0.0;
    float roughness = 0.0;
    float3 emissive = float3(0.0, 0.0, 0.0);

    RayDesc rayDesc;
    Payload payload;
    payload.miss_frag = false;
    payload.hitPoint = float3(0.0, 0.0, 0.0);
    payload.hitTangent = float3(0.0, 0.0, 0.0);
    payload.hitNormal = float3(0.0, 0.0, 0.0);
    payload.radiance = float3(0.0, 0.0, 0.0);

    int sampleCount = 225;
    float3 radiance = float3(0.0, 0.0, 0.0);

    float3 throughput = float3(1.0, 1.0, 1.0);

    for (int sy = 0; sy < sqrt(sampleCount); sy++) {
        for (int sx = 0; sx < sqrt(sampleCount); sx++) {

            uint sampleIndex = sy * (int)sqrt(sampleCount) + sx;
            payload.seed = Hash_Wang(seed ^ sampleIndex);
            payload.depth = 0;

            float2 jitter = sampleDisk(state) * (1.0 / float2(launchSize));
            float2 ndcJ = float2(2.0 * (pixel.x + jitter.x) - 1.0, -2.0 * (pixel.y + jitter.y) + 1.0);
            float3 dir = normalize(forward +
                                   ndcJ.x * aspect * t * right +
                                   ndcJ.y * t * up);

            rayDesc.Origin = CamPos.xyz;
            rayDesc.Direction = dir;
            rayDesc.TMin = 0.001;
            rayDesc.TMax = 1e6;

            TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, payload);
            if (payload.miss_frag) {
                radiance = payload.radiance;
                outputTexture[launchIndex] = float4(radiance, 1.0);
                return;
            }

            radiance += payload.radiance;

            for (int d = 0; d < max_depth; ++d) {
                float3 hitPos = payload.hitPoint;
                float eps = max(1e-4, 1e-3 * length(hitPos));

                RayDesc nextRayDesc;
                nextRayDesc.Origin = payload.hitPoint + payload.nextRay * eps;
                nextRayDesc.Direction = payload.nextRay;
                nextRayDesc.TMax = 1e6;
                nextRayDesc.TMin = 0.001;

                TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 1, 0, 1, nextRayDesc, payload);

                radiance += float3(
                    throughput.r * payload.radiance.r,
                    throughput.g * payload.radiance.g,
                    throughput.b * payload.radiance.b
                );
                throughput = float3(
                    throughput.r * payload.radiance.r,
                    throughput.g * payload.radiance.g,
                    throughput.b * payload.radiance.b
                );
            }
        }
    }
    radiance /= sampleCount;

    outputTexture[launchIndex] = float4(radiance, 1.0);
    return;
}