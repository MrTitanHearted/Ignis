module Ignis;

struct Vertex {
    [[vk::location(0)]]
    float3 Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
    [[vk::location(3)]]
    float4 Tangent;
};

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float3 FragmentPosition;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
    [[vk::location(3)]]
    float3 Tangent;
    [[vk::location(4)]]
    float Bitangent;
    [[vk::location(5)]]
    uint Material;
};

typedef VertexOutput FragmentInput;

const static float k_PI = 3.14159265358979323846f;

const static uint k_InvalidTexture = ~0u;

struct Material {
    float3 AlbedoFactor;

    float MetallicFactor;
    float RoughnessFactor;

    uint AlbedoTexture;
    uint NormalTexture;
    uint MetallicRoughnessTexture;
};

struct DirectionalLight {
    float3 Direction;
    float3 Color;
};

struct PointLight {
    float3 Position;
    float Constant;
    float3 Color;
    float Linear;

    float Quadratic;

    float _ignis_padding[3];
};

struct SpotLight {
    float3 Position;
    float Constant;
    float3 Direction;
    float Linear;
    float3 Color;
    float Quadratic;

    float CutOff;
    float OuterCutOff;

    float _ignis_padding[2];
};

struct LightData {
    uint PointLightCount;
    uint SpotLightCount;
};

struct Mesh {
    float4x4 MeshTransform;
    float4x4 NormalTransform;

    uint Material;
    uint _ignis_padding[3];
};

struct Instance {
    float4x4 ModelTransform;
    float4x4 NormalTransform;
};

[[vk::binding(0, 0)]]
Sampler2D gTextures[];
[[vk::binding(1, 0)]]
StructuredBuffer<Material> gMaterials;

[[vk::binding(0, 1)]]
ConstantBuffer<DirectionalLight> gDirectionLight;
[[vk::binding(1, 1)]]
StructuredBuffer<PointLight> gPointLights;
[[vk::binding(2, 1)]]
StructuredBuffer<SpotLight> gSpotLights;
[[vk::binding(3, 1)]]
ConstantBuffer<LightData> gLightData;

[[vk::binding(0, 2)]]
StructuredBuffer<Mesh> gModelMeshes[];
[[vk::binding(1, 2)]]
StructuredBuffer<Instance> gModelInstances[];

struct DrawPC {
    float4x4 ProjectionView;

    float3 ViewPosition;

    uint Model;
};

[[vk::push_constant]]
ConstantBuffer<DrawPC> gDrawPC;

[shader("vertex")]
VertexOutput vs_main(
    Vertex input,
    uint   instance_index: SV_InstanceID,
    uint   mesh_index: SV_DrawIndex) {
    Mesh mesh = gModelMeshes[gDrawPC.Model].Load(mesh_index);

    Instance instance = gModelInstances[gDrawPC.Model].Load(instance_index);

    float4x4 model  = instance.ModelTransform;
    float3x3 normal = float3x3(instance.NormalTransform);

    model  = mul(model, mesh.MeshTransform);
    normal = mul(normal, float3x3(mesh.NormalTransform));

    VertexOutput output;

    output.Position = mul(gDrawPC.ProjectionView, mul(model, float4(input.Position, 1.0f)));

    output.FragmentPosition = mul(model, float4(input.Position, 1.0f)).xyz;

    output.Normal = mul(normal, input.Normal);
    output.UV     = input.UV;

    output.Tangent   = mul(normal, input.Tangent.xyz);
    output.Bitangent = input.Tangent.w;

    output.Material = mesh.Material;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    Material material = gMaterials[input.Material];

    float3 material_albedo = material.AlbedoFactor;
    float3 material_normal = normalize(input.Normal);

    float material_metallic  = material.MetallicFactor;
    float material_roughness = material.RoughnessFactor;

    if (k_InvalidTexture != material.AlbedoTexture) {
        material_albedo *= gTextures[material.AlbedoTexture].Sample(input.UV).rgb;
    }
    if (k_InvalidTexture != material.NormalTexture) {
        float3 n = material_normal;
        float3 t = normalize(input.Tangent);
        float3 b = cross(n, t) * input.Bitangent;

        float3x3 tbn = float3x3(t, b, n);

        material_normal = gTextures[material.NormalTexture].Sample(input.UV).rgb;

        material_normal = normalize(mul(tbn, material_normal * 2.0f - 1.0f));
    }
    if (k_InvalidTexture != material.MetallicRoughnessTexture) {
        float2 material_metallic_roughness = gTextures[material.MetallicRoughnessTexture].Sample(input.UV).gb;

        material_metallic *= material_metallic_roughness.g;
        material_roughness *= material_metallic_roughness.r;
    }

    // return float4(material_albedo, 1.0f);
    return float4(material_normal * 0.5f + 0.5f, 1.0f);
}

