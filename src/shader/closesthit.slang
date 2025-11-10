#include "common_types.slang"
#include "random.slang"
#include "util.slang"

[shader("closesthit")]
void closestHitMain(
    in BuiltInTriangleIntersectionAttributes attr,
    inout Payload payload
) {
    uint state = Hash_Wang(payload.seed);

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

    payload.normal = N;

    float2 uv = vertices[i0].texCoord.xy * w +
                vertices[i1].texCoord.xy * u +
                vertices[i2].texCoord.xy * v;

    payload.uv = uv;

    float3 p0 = vertices[i0].pos.xyz;
    float3 p1 = vertices[i1].pos.xyz;
    float3 p2 = vertices[i2].pos.xyz;

    float2 uv0 = vertices[i0].texCoord.xy;
    float2 uv1 = vertices[i1].texCoord.xy;
    float2 uv2 = vertices[i2].texCoord.xy;

    // ここから tangent / bitangent 計算（トライアングル単位）
    float3 dp1 = p1 - p0;
    float3 dp2 = p2 - p0;
    float2 duv1 = uv1 - uv0;
    float2 duv2 = uv2 - uv0;

    float r = duv1.x * duv2.y - duv1.y * duv2.x;
    float3 T, B;

    if (abs(r) > 1e-8)
    {
        float invR = 1.0 / r;
        T = (dp1 * duv2.y - dp2 * duv1.y) * invR;
        B = (dp2 * duv1.x - dp1 * duv2.x) * invR;
    }
    else
    {
        T = float3(1, 0, 0);
        B = cross(N, T);
    }

    // 法線に直交化しておくとノーマルマップに優しい
    T = normalize(T - N * dot(N, T));
    B = normalize(cross(N, T));

    float3 pObj = vertices[i0].pos.xyz * w +
                  vertices[i1].pos.xyz * u +
                  vertices[i2].pos.xyz * v;

    float3 hitPos = mul(float4(pObj, 1.0), ObjectToWorld4x3()).xyz;
    float3 inRay = -WorldRayDirection();
    float3 Ns = normalize(mul(N, (float3x3)ObjectToWorld3x4()));
    float3 Reflect = reflect(inRay, Ns);
    float eps = max(1e-4, 1e-3 * length(hitPos));

    float3 baseColor = float3(0.0, 0.0, 0.0);
    float transmission = 1.0;
    float ior = 0.0;
    float metallic = 0.0;
    float roughness = 0.0;
    float3 emissive = float3(0.0, 0.0, 0.0);

    uint matId = primitiveMat[prim];
    Material m = materials[matId];

    baseColor = m.baseColorFactor.rgb;
    transmission = m.transmission;
    ior = m.ior;
    metallic = m.metallicFactor;
    roughness = m.roughnessFactor;
    emissive = m.emissiveFactor.rgb;

    if (m.baseColorTextureIndex != -1) {
        int index = m.baseColorTextureIndex;
        float4 color = textures[index].SampleLevel(texSampler, uv, 0.0).rgba;
        baseColor *= color.rgb;
    }

    if (m.matallicRoughnessTextureIndex != -1) {
        int index = m.matallicRoughnessTextureIndex;
        float2 metalRough = textures[index].SampleLevel(texSampler, uv, 0.0).rg;
        metallic *= metalRough.r;
        roughness *= metalRough.g;
    }

    payload.depth += 1;
    if (payload.depth >= 5) {
        payload.radiance = emissive;
        return;
    }

    float3 albedo = float3(0.0, 0.0, 0.0);

    if (metallic > 0.0) {
        float roulette = rand(payload.seed);
        if (roulette < 0.5) {
            // diffuse
            float3 i = -WorldRayDirection();
            float rnd_u1 = rand(state);
            float cos_t = sqrt(rnd_u1);
            float sin_t = sqrt(max(1 - rnd_u1, 0));
            float phi = 2.0 * PI * rand(state);
            float3 dir = normalize(sin_t * cos(phi) * T + sin_t * sin(phi) * B + cos_t * N);

            RayDesc rayDesc;
            rayDesc.Origin = hitPos + Reflect * eps;
            rayDesc.Direction = dir;
            rayDesc.TMin = 0.001;
            rayDesc.TMax = 1e6;

            TraceRay(topLevelAS,
                     RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 1, 0, 1, rayDesc, payload);

            albedo += payload.radiance;
        }else{
            // specular(GGX)
            // i: in, o: out, m: half
            float3 i = worldToLocal(-WorldRayDirection(), payload.normal);
            float3 m = sampleGGX(roughness, payload.seed);
            float3 o = reflect(-i, m);

            RayDesc rayDesc;
            rayDesc.Origin = hitPos + Reflect * eps;
            rayDesc.Direction = localToWorld(o, payload.normal);
            rayDesc.TMin = 0.001;
            rayDesc.TMax = 1e6;

            TraceRay(topLevelAS,
                     RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                     0xFF, 1, 0, 1, rayDesc, payload);
        }
    }




    payload.radiance = emissive + 0.7 * baseColor;
}