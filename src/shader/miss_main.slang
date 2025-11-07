#include "common_types.slang"

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.miss_frag = true;
    float3 dir = normalize(WorldRayDirection());

    // equirectangular環境マップをUVに変換
    float u = (atan2(dir.z, dir.x) / (2.0 * 3.14)) + 0.5;
    float v = acos(clamp(dir.y, -1.0, 1.0)) / 3.14;

    // テクスチャからフェッチ
    float4 envColor = envMapTex.SampleLevel(envSampler, dir, 0.0);

    payload.radiance = envColor.rgb;
}