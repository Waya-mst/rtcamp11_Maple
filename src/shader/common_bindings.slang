public struct Vertex {
    float4 pos;
    float4 normal;
    float4 texCoord;
}

[vk::binding(0,0)] RaytracingAccelerationStructure topLevelAS;
[vk::binding(1,0)] RWTexture2D<float4> outputTexture;
[vk::binding(2,0)] cbuffer SceneUBO {
    float4 SunDir;
    float4 SunColor;
    float4 CamPos;
};
[vk::binding(3,0)] StructuredBuffer<Vertex> vertices;
[vk::binding(4,0)] StructuredBuffer<uint>   indices;
[vk::binding(5,0)] Texture2D<float4>        BaseColorTex;
[vk::binding(6,0)] SamplerState             BaseSampler;