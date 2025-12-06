module Ignis;

struct Vertex {
    [[vk::location(0)]]
    float3 Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
};

struct VertexOutput {
    [[vk::location(0)]]
    float4 Position : SV_Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
    [[vk::location(3)]]
    uint MaterialID;
};

typedef VertexOutput FragmentInput;

struct SampledTexture {
    uint TextureID;
    uint SamplerID;
};

struct Material {
    SampledTexture DiffuseTexture;
    SampledTexture SpecularTexture;

    float Shininess;
};

struct Camera {
    float4x4 Projection;
    float4x4 View;
};

[[vk::binding(0, 0)]]
SamplerState gSamplers[];
[[vk::binding(1, 0)]]
Texture2D gTextures[];
[[vk::binding(2, 0)]]
StructuredBuffer<Material> gMaterials;
[[vk::binding(3, 0)]]
ConstantBuffer<Camera> gCamera;

struct DrawData {
    uint MaterialID;

    uint _ignis_padding[3];

    float4x4 Model;
};

[[vk::push_constant]]
DrawData gDrawData;

[shader("vertex")]
VertexOutput vs_main(Vertex input) {
    VertexOutput output;

    output.Position = mul(gCamera.Projection, mul(gCamera.View, mul(gDrawData.Model, float4(input.Position, 1.0f))));
    output.Normal   = mul(float3x3(gDrawData.Model), input.Normal);
    output.UV       = input.UV;

    output.MaterialID = gDrawData.MaterialID;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    Material material = gMaterials[input.MaterialID];

    Texture2D diffuse_texture = gTextures[material.DiffuseTexture.TextureID];
    SamplerState diffuse_sampler = gSamplers[material.DiffuseTexture.SamplerID];

    return diffuse_texture.Sample(diffuse_sampler, input.UV);
}

