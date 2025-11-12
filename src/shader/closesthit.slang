#include "common_types.slang"
#include "random.slang"
#include "util.slang"

[shader("closesthit")]
void closestHitMain(
    in BuiltInTriangleIntersectionAttributes attr,
    inout Payload payload
) {
    //uint state = Hash_Wang(payload.seed);

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

    payload.hitNormal = N;

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

    T = normalize(T - N * dot(N, T));
    B = normalize(cross(N, T));

    payload.hitTangent = T;
    payload.hitBitangent = B;

    float3 pObj = vertices[i0].pos.xyz * w +
                  vertices[i1].pos.xyz * u +
                  vertices[i2].pos.xyz * v;

    float3 hitPos = mul(float4(pObj, 1.0), ObjectToWorld4x3()).xyz;
    float3 inRay = -WorldRayDirection();
    float3 Ns = normalize(mul(N, (float3x3)ObjectToWorld3x4()));
    float3 Reflect = reflect(inRay, Ns);
    float eps = max(1e-4, 1e-3 * length(hitPos));

    payload.hitPoint = hitPos;
    payload.inRay = -WorldRayDirection();

    float3 baseColor = float3(0.0, 0.0, 0.0);
    float transmission = 1.0;
    float ior = 0.0;
    float metallic = 0.0;
    float roughness = 0.0;
    float3 emissive = float3(0.0, 0.0, 0.0);

    uint matId = primitiveMat[prim];
    Material m = materials[matId];

    payload.material = m;

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
    if (payload.depth >= max_depth + 1) {
        payload.radiance = emissive;
        return;
    }

    float3 albedo = float3(0.0, 0.0, 0.0);

    float3 prv_throughput = payload.throughput;
    float3 cur_throughput = prv_throughput * baseColor;

    if (metallic > 0.01) {
        // specular(GGX)
        // i: in, o: out, m: half
        float3 i = worldToLocal(-WorldRayDirection(), payload.hitNormal);
        float3 m = sampleGGX(roughness, payload.seed);
        float3 o = reflect(-i, m);

        payload.nextRay = localToWorld(o, payload.hitNormal);

        float no = abs(cosTheta(o));
        float ni = abs(cosTheta(i));
        float nm = abs(cosTheta(m));
        float mi = abs(dot(i, m));

        float3 F = baseColor;
        float G = ggxGeometry(i, o, m, roughness);

        float3 weight = F * G * dot(o, m) / (dot(i, payload.hitNormal) * dot(m, payload.hitNormal));
        payload.radiance = emissive + weight * envMapTex.SampleLevel(envSampler, o, 0.0).rgb;

    } else {
        // diffuse
        float3 dir = sampleHemisphereCosine(Ns, payload.seed);
        payload.nextRay = dir;

        payload.radiance = prv_throughput * emissive + cur_throughput * envMapTex.SampleLevel(envSampler, dir, 0.0).rgb;
        payload.throughput = cur_throughput;
    }
}