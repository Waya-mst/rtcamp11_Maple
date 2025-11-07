#pragma once
#include "common_bindings.slang"

public static const float PI = 3.1415926535;

public static const float3 kTarget = float3(0,0,0);
public static const float3 kUp     = float3(0,1,0);
public static const float kVFov = radians(45.0);

public float3 scene(
    float4 SunDir,
    uint2 ls,
    float3 camPos
);

public struct ShadowPayload { public bool occluded; };

public struct Payload
{
    public uint seed;
    public uint depth;
    public bool miss_frag;
    public float3 radiance;
    public Material material;
    public float3 normal;
    public float2 uv;
};
