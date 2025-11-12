#pragma once
#include "random.slang"

float cosTheta(float3 w) {
    return w.z;
}

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

// 間違えてるかも
float pre_ggxGeometry(float3 v, float3 n, float a) {
    float t = acos(dot(v, n));
    float x = step(0.0, dot(v, n) / v.z);
    return x * 2.0 / (1.0 + sqrt(1 + a * a * t * t));
}

float ggxGeometry(float3 i, float3 o, float3 m, float roughness) {
    float a = roughness * roughness;
    return pre_ggxGeometry(i, m, a) * pre_ggxGeometry(o, m, a);
}

float3 sampleHemisphereCosine(float3 normal, inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);

    float phi = 2.0 * PI * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 direction;
    direction.x = cos(phi) * sinTheta;
    direction.y = sin(phi) * sinTheta;
    direction.z = cosTheta;

    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    float3 sampleDirection = tangent * direction.x + bitangent * direction.y + normal * direction.z;
    return normalize(sampleDirection);
}