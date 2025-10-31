public static const float3 kTarget = float3(0,0,0);
public static const float3 kUp     = float3(0,1,0);
public static const float   kVFov  = radians(45.0);

public float3 scene(
    float4 SunDir,
    uint2 ls,
    float3 camPos
);

public struct RadiancePayload { public float3 radiance; };
public struct ShadowPayload { public bool occluded; };

public struct Payload
{
    public float3 color;
};
