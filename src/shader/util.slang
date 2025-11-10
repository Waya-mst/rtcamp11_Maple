#pragma once
#include "random.slang"

float3 worldToLocal(float3 worldDir, float3 normal) {
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    float3 localDir;
    localDir.x = dot(worldDir, tangent);
    localDir.y = dot(worldDir, bitangent);
    localDir.z = dot(worldDir, normal);
    return localDir;
}

float3 localToWorld(float3 localDir, float3 normal) {
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    float3 worldDir;
    worldDir.x = dot(localDir, tangent);
    worldDir.y = dot(localDir, bitangent);
    worldDir.z = dot(localDir, normal);
    return worldDir;
}

float3 sampleGGX(float roughness, inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);
    float alpha = roughness * roughness;
    float theta = atan(alpha * sqrt(v) / sqrt(1.0 - v));
    float phi = 2.0 * PI * u;
    return float3(sin(phi) * sin(theta), cos(phi) * sin(theta), cos(theta));
}