#include "common_types.slang"

uint Hash_Wang(uint key) {
    key = (key ^ 61u) ^ (key >> 16u);
    key = key + (key << 3u);
    key = key ^ (key >> 4u);
    key = key * 0x27D4EB2Du;
    key = key ^ (key >> 15u);
    return key;
}

public uint pcg(uint v)  
{  
    uint state = v * 747796405 + 2891336453;  
    uint word = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;  
    return word ^ word >> 22;  
}

public uint2 pcg2d(uint2 v)
{
    v = v * 1664525 + 1013904223;

    v.x += v.y * 1664525;
    v.y += v.x * 1664525;

    v = v ^ v >> 16;

    v.x += v.y * 1664525;
    v.y += v.x * 1664525;

    v = v ^ v >> 16;

    return v;
}

public float rand(inout uint seed) {
    uint val = pcg(seed);
    return (float(val) * (1.0 / float(0xffffffffu)));
}

public float randRange(inout uint seed, float low, float high) {
    float t = rand(seed);
    return lerp(low, high, t);
}

public float2 sampleDisk(inout uint seed) {
    float u = rand(seed); // rad
    float v = rand(seed); // arg
    float theta = 2.0 * PI * v;
    float2 pos;
    pos.x = sqrt(u) * cos(theta);
    pos.y = sqrt(u) * sin(theta);
    return pos;
}