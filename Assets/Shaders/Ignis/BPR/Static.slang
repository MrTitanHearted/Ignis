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
    uint Material;
};

typedef VertexOutput FragmentInput;

struct Material {
    uint DiffuseTexture;
    uint SpecularTexture;

    float Shininess;
};

struct Instance {
    column_major float4x4 ModelTransform;
    column_major float4x4 NormalTransform;
};

struct Camera {
    column_major float4x4 Projection;
    column_major float4x4 View;
};

[[vk::binding(0, 0)]]
Sampler2D gTextures[];
[[vk::binding(1, 0)]]
StructuredBuffer<Material> gMaterials;
[[vk::binding(2, 0)]]
StructuredBuffer<Instance> gInstances[];
[[vk::binding(3, 0)]]
ConstantBuffer<Camera> gCamera;

struct DrawData {
    uint Material;
    uint Instance;
};

[[vk::push_constant]]
DrawData gDrawData;

[shader("vertex")]
VertexOutput vs_main(Vertex input, uint instance_index: SV_InstanceID) {
    Instance instance = gInstances[gDrawData.Instance].Load(instance_index);

    column_major float4x4 model  = instance.ModelTransform;
    column_major float4x4 normal = instance.NormalTransform;

    VertexOutput output;

    output.Position = mul(gCamera.Projection, mul(gCamera.View, mul(model, float4(input.Position, 1.0f))));
    // output.Position = mul(gCamera.Projection, mul(gCamera.View, float4(input.Position, 1.0f)));
    output.Normal   = mul(float3x3(normal), input.Normal);
    output.UV       = input.UV;

    output.Material = gDrawData.Material;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    Material material = gMaterials[input.Material];

    Sampler2D diffuse_texture = gTextures[material.DiffuseTexture];

    return diffuse_texture.Sample(input.UV);
}

