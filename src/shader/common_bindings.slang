public struct Vertex {
    float4 pos;
    float4 normal;
    float4 texCoord;
}

public struct Material {
    int baseColorTextureIndex;
    int matallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occulusionTextureIndex;
    int emissiveTextureIndex;

    float metallicFactor;
    float roughnessFactor;
    float ior;

    float4 baseColorFactor;
    float4 emissiveFactor;
}

[vk::binding(0,0)] RaytracingAccelerationStructure topLevelAS;
[vk::binding(1,0)] RWTexture2D<float4> outputTexture;
[vk::binding(2,0)] cbuffer SceneUBO {
    float4 SunDir;
    float4 SunColor;
    float4 CamPos;
};
[vk::binding(3,0)] StructuredBuffer<Vertex> vertices;
[vk::binding(4,0)] StructuredBuffer<uint> indices;
[vk::binding(5,0)] StructuredBuffer<Material> materials;
[vk::binding(6,0)] StructuredBuffer<uint> primitiveMat;
[vk::binding(7,0)] Texture2D<float4>        BaseColorTex;
[vk::binding(8,0)] SamplerState BaseSampler;
[vk::binding(9,0)] TextureCube<float4> envMapTex;
[vk::binding(10,0)] SamplerState envSampler;