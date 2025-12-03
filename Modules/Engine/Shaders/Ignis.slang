public module Ignis;

struct Vertex {
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 UV;
    float4 Color;
    uint4  Joint;
    float4 Weight;
};

struct VertexOutput {
    [[vk::location(0)]]
    float4 Position : SV_Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float3 Tangent;
    [[vk::location(3)]]
    float2 UV;
    [[vk::location(4)]]
    float4 Color;
    [[vk::location(5)]]
    uint4 Joint;
    [[vk::location(6)]]
    float4 Weight;
};

struct FragmentInput {
    [[vk::location(0)]]
    float4 Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float3 Tangent;
    [[vk::location(3)]]
    float2 UV;
    [[vk::location(4)]]
    float4 Color;
    [[vk::location(5)]]
    uint4 Joint;
    [[vk::location(6)]]
    float4 Weight;
};

struct SampledTexture {
    uint TextureID;
    uint SamplerID;
};

struct Camera {
    float4x4 Projection;
    float4x4 View;
};

struct Mesh {
    uint VertexBufferID;
    uint IndexBufferID;
};

struct Material {
    SampledTexture DiffuseTexture;
    SampledTexture NormalTexture;
    SampledTexture SpecularTexture;

    float4 DiffuseColor;
    float4 NormalColor;
    float4 SpecularColor;

    float Shininess;
};

[[vk::binding(0, 0)]]
Texture2D gTextures[];
[[vk::binding(1, 0)]]
SamplerState gSamplers[];
[[vk::binding(2, 0)]]
StructuredBuffer<Vertex> gVertexBuffers[];
[[vk::binding(3, 0)]]
StructuredBuffer<uint> gIndexBuffers[];
[[vk::binding(4, 0)]]
ConstantBuffer<Camera> gCameras[];
[[vk::binding(5, 0)]]
ConstantBuffer<Mesh> gMeshes[];
[[vk::binding(6, 0)]]
ConstantBuffer<Material> gMaterials[];

struct DrawData {
    uint CameraID;
    uint MeshID;
    uint MaterialID;

    float4x4 Model;
};

[[vk::push_constant]]
DrawData gDrawData;
