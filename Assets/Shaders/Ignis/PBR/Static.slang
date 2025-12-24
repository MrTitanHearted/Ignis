module Ignis;

struct Vertex {
    [[vk::location(0)]]
    float3 Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
    [[vk::location(3)]]
    float3 Tangent;
    [[vk::location(4)]]
    float3 Bitangent;
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
    float3 Bitangent;
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
    float  _ignis_padding0;
    float3 Color;
    float  _ignis_padding1;
};

struct PointLight {
    float3 Position;
    float  Constant;
    float3 Color;
    float  Linear;

    float Quadratic;

    float _ignis_padding[3];
};

struct SpotLight {
    float3 Position;
    float  Constant;
    float3 Direction;
    float  Linear;
    float3 Color;
    float  Quadratic;

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
[[vk::binding(2, 0)]]
SamplerCube gSkyboxTexture;

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

    output.Tangent   = mul(normal, input.Tangent);
    output.Bitangent = mul(normal, input.Bitangent);

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
        material_albedo *= pow(gTextures[material.AlbedoTexture].Sample(input.UV).rgb, 2.2f);
    }
    if (k_InvalidTexture != material.NormalTexture) {
        float3 tangent_normal = gTextures[material.NormalTexture].Sample(input.UV).rgb * 2.0f - 1.0f;

        float3 q1  = ddx(input.FragmentPosition);
        float3 q2  = ddy(input.FragmentPosition);
        float2 st1 = ddx(input.UV);
        float2 st2 = ddy(input.UV);

        float3 n = normalize(input.Normal);
        float3 t = normalize(q1 * st2.g - q2 * st1.g);
        float3 b = -normalize(cross(n, t));

        float3x3 tbn = float3x3(t, b, n);

        material_normal = normalize(mul(tbn, tangent_normal));
    }
    if (k_InvalidTexture != material.MetallicRoughnessTexture) {
        float2 material_metallic_roughness = gTextures[material.MetallicRoughnessTexture].Sample(input.UV).gb;

        material_metallic *= material_metallic_roughness.g;
        material_roughness *= material_metallic_roughness.r;
    }

    float3 result = float3(0.0f);

    if (true) {
        float3 I = normalize(input.FragmentPosition - gDrawPC.ViewPosition);
        float3 R = reflect(I, material_normal);

        result = gSkyboxTexture.Sample(R).rgb;
    }

    if (false) {
        float ratio = 1.0f / 1.52f;

        float3 I = normalize(input.FragmentPosition - gDrawPC.ViewPosition);
        float3 R = refract(I, material_normal, ratio);

        result = gSkyboxTexture.Sample(R).rgb;
    }

    return float4(result, 1.0f);
}

float3 EvaluateBRDF(
    float  d,
    float3 f,
    float  g,
    float  n_dot_v,
    float  n_dot_l,
    float3 radiance,
    float3 albedo,
    float  metallic) {
    n_dot_v = max(n_dot_v, 0.000001f);
    n_dot_l = max(n_dot_l, 0.000001f);

    float3 specular = (d * f * g) / (4.0f * n_dot_v * n_dot_l);

    float3 k_d = (1.0f - f) * (1.0f - metallic);

    float3 diffuse = k_d * albedo / k_PI;

    return (diffuse + specular) * radiance * n_dot_l;
}

float CalculateDistributionGGX(float n_dot_h, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;

    float n_dot_h2 = n_dot_h * n_dot_h;

    float denominator = (n_dot_h2 * (a2 - 1.0f) + 1.0f);

    denominator = k_PI * denominator * denominator;

    return a2 / denominator;
}

float3 CalculateFresnelSchlick(float cos_theta, float3 f0) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float3 CalculateF0Metallic(float3 albedo, float metallic) {
    return lerp(float3(0.04f), albedo, metallic);
}

float CalculateGeometrySmith(float n_dot_v, float n_dot_l, float roughness) {
    float ggx1 = CalculateGeometrySchlickGGX(n_dot_l, roughness);
    float ggx2 = CalculateGeometrySchlickGGX(n_dot_v, roughness);

    return ggx1 * ggx2;
}

float CalculateGeometrySchlickGGX(float n_dot_v, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float denominator = n_dot_v * (1.0f - k) + k;

    return n_dot_v / denominator;
}

