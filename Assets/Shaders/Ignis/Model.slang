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
    float2 UV;
    [[vk::location(2)]]
    float3 Normal;
    [[vk::location(3)]]
    float4 Tangent;
    [[vk::location(4)]]
    uint Material;
};

typedef VertexOutput FragmentInput;

const static float k_PI = 3.14159265358979323846f;

const static uint k_InvalidTexture = ~0u;

struct Material {
    float3 AlbedoFactor;
    float  MetallicFactor;
    float3 EmissiveFactor;
    float  RoughnessFactor;

    uint AlbedoTexture;
    uint NormalTexture;
    uint EmissiveTexture;
    uint AmbientOcclusionTexture;

    uint MetallicRoughnessTexture;
    uint MetallicTexture;
    uint RoughnessTexture;

    float _ignis_padding;
};

struct DirectionalLight {
    float3 Direction;
    float  Power;
    float3 Color;
    float  _ignis_padding;
};

struct PointLight {
    float3 Position;
    float  Power;
    float3 Color;

    float _ignis_padding;
};

struct SpotLight {
    float3 Position;
    float  Power;
    float3 Direction;
    float  CutOff;
    float3 Color;
    float  OuterCutOff;
};

struct LightData {
    uint PointLightCount;
    uint SpotLightCount;

    float _ignis_padding[2];
};

struct Mesh {
    float4x4 VertexTransform;
    float4x4 NormalTransform;

    uint Material;

    float _ignis_padding[3];
};

struct Instance {
    float4x4 VertexTransform;
    float4x4 NormalTransform;
};

[[vk::binding(0, 0)]]
Sampler2D gTextures[];
[[vk::binding(1, 0)]]
SamplerCube gSkyboxTexture;
[[vk::binding(2, 0)]]
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

    float3x3 mesh_normal_transform     = float3x3(mesh.NormalTransform);
    float3x3 instance_normal_transform = float3x3(instance.NormalTransform);

    float4x4 model_transform  = mul(instance.VertexTransform, mesh.VertexTransform);
    float3x3 normal_transform = mul(instance_normal_transform, mesh_normal_transform);

    float4 position = mul(model_transform, float4(input.Position, 1.0f));

    float handedness = input.Tangent.w;

    float3 normal  = normalize(mul(normal_transform, input.Normal));
    float3 tangent = normalize(mul(normal_transform, input.Tangent.xyz));

    VertexOutput output;

    output.Position = mul(gDrawPC.ProjectionView, position);

    output.FragmentPosition = position.xyz;

    output.UV = input.UV;

    output.Normal  = normal;
    output.Tangent = float4(tangent, handedness);

    output.Material = mesh.Material;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    Material material = gMaterials[input.Material];

    float3 material_albedo   = material.AlbedoFactor;
    float3 material_normal   = normalize(input.Normal);
    float3 material_emission = material.EmissiveFactor;
    float3 material_ao       = float3(1.0f);

    float material_metallic  = material.MetallicFactor;
    float material_roughness = material.RoughnessFactor;

    if (k_InvalidTexture != material.AlbedoTexture) {
        material_albedo *= gTextures[material.AlbedoTexture].Sample(input.UV).rgb;
    }
    if (k_InvalidTexture != material.NormalTexture) {
        float3 tangent_normal = gTextures[material.NormalTexture].Sample(input.UV).rgb;

        tangent_normal = normalize(tangent_normal * 2.0f - 1.0f);

        float3 N = normalize(input.Normal);
        float3 T = normalize(input.Tangent.xyz);

        T = normalize(T - N * dot(N, T));

        float handedness = input.Tangent.w;

        float3 B = -normalize(handedness * cross(T, N));

        float3x3 TBN = transpose(float3x3(T, B, N));

        material_normal = mul(TBN, tangent_normal);
    }
    if (k_InvalidTexture != material.EmissiveTexture) {
        material_emission *= gTextures[material.EmissiveTexture].Sample(input.UV).rgb;
    }
    if (k_InvalidTexture != material.AmbientOcclusionTexture) {
        material_ao *= gTextures[material.AmbientOcclusionTexture].Sample(input.UV).rgb;
    }
    if (k_InvalidTexture != material.MetallicRoughnessTexture) {
        float2 material_metallic_roughness = gTextures[material.MetallicRoughnessTexture].Sample(input.UV).gb;

        material_metallic *= material_metallic_roughness.g;
        material_roughness *= material_metallic_roughness.r;
    } else {
        if (k_InvalidTexture != material.MetallicTexture) {
            material_metallic *= gTextures[material.MetallicTexture].Sample(input.UV).r;
        }
        if (k_InvalidTexture != material.RoughnessTexture) {
            material_roughness *= gTextures[material.RoughnessTexture].Sample(input.UV).r;
        }
    }

    // PBR lighting calculation
    float3 N = normalize(material_normal);
    float3 V = normalize(gDrawPC.ViewPosition - input.FragmentPosition);

    float3 result = float3(0.0f);

    float3 f0 = GetF0Metallic(material_albedo, material_metallic);

    // Directional light
    {
        float3 L = normalize(-gDirectionLight.Direction);
        float3 H = normalize(V + L);

        float n_dot_v = max(dot(N, V), 0.0f);
        float n_dot_l = max(dot(N, L), 0.0f);
        float n_dot_h = max(dot(N, H), 0.0f);
        float v_dot_h = max(dot(V, H), 0.0f);

        float  D = CalculateDistributionGGX(n_dot_h, material_roughness);
        float3 F = CalculateFresnelSchlick(v_dot_h, f0);
        float  G = CalculateGeometrySmith(n_dot_v, n_dot_l, material_roughness);

        float3 kS = F;
        float3 kD = 1.0f - kS;

        float3 diffuse_brdf  = EvaluateDiffuseBRDF(kD, material_albedo);
        float3 specular_brdf = EvaluateSpecularBRDF(kS, D, F, G, n_dot_l, n_dot_v);

        float3 radiance = gDirectionLight.Color * gDirectionLight.Power;

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, n_dot_l);
    }

    for (uint i = 0; i < gLightData.PointLightCount; i++) {
        float3 LightToFragment = gPointLights[i].Position - input.FragmentPosition;

        float3 L = normalize(LightToFragment);
        float3 H = normalize(V + L);

        float n_dot_v = max(dot(N, V), 0.0f);
        float n_dot_l = max(dot(N, L), 0.0f);
        float n_dot_h = max(dot(N, H), 0.0f);
        float v_dot_h = max(dot(V, H), 0.0f);

        float  D = CalculateDistributionGGX(n_dot_h, material_roughness);
        float3 F = CalculateFresnelSchlick(v_dot_h, f0);
        float  G = CalculateGeometrySmith(n_dot_v, n_dot_l, material_roughness);

        float3 kS = F;
        float3 kD = 1.0f - kS;

        float3 diffuse_brdf  = EvaluateDiffuseBRDF(kD, material_albedo);
        float3 specular_brdf = EvaluateSpecularBRDF(kS, D, F, G, n_dot_l, n_dot_v);

        float attenuation = length(LightToFragment);

        float3 radiance = gPointLights[i].Color * gPointLights[i].Power / attenuation * attenuation;

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, n_dot_l);
    }

    for (uint i = 0; i < gLightData.SpotLightCount; i++) {
        float3 LightToFragment = gSpotLights[i].Position - input.FragmentPosition;

        float3 L = normalize(LightToFragment);
        float3 H = normalize(V + L);

        float n_dot_v = max(dot(N, V), 0.0f);
        float n_dot_l = max(dot(N, L), 0.0f);
        float n_dot_h = max(dot(N, H), 0.0f);
        float v_dot_h = max(dot(V, H), 0.0f);

        float  D = CalculateDistributionGGX(n_dot_h, material_roughness);
        float3 F = CalculateFresnelSchlick(v_dot_h, f0);
        float  G = CalculateGeometrySmith(n_dot_v, n_dot_l, material_roughness);

        float3 kS = F;
        float3 kD = 1.0f - kS;

        float3 diffuse_brdf  = EvaluateDiffuseBRDF(kD, material_albedo);
        float3 specular_brdf = EvaluateSpecularBRDF(kS, D, F, G, n_dot_l, n_dot_v);

        float theta     = dot(L, normalize(-gSpotLights[i].Direction));
        float epsilon   = gSpotLights[i].CutOff - gSpotLights[i].OuterCutOff;
        float intensity = clamp((theta - gSpotLights[i].OuterCutOff) / epsilon, 0.0f, 1.0f);

        float attenuation = length(LightToFragment);

        float3 radiance = gSpotLights[i].Color * gSpotLights[i].Power * intensity / attenuation * attenuation;

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, n_dot_l);
    }

    // Add ambient occlusion and emission
    float3 ambient = float3(0.03f) * material_albedo * material_ao;
    result += ambient + material_emission;

    result = result / (result + 1.0f);

    return float4(result, 1.0f);
}

float3 EvaluateBRDF(
    float3 diffuse_brdf,
    float3 specular_brdf,
    float3 radiance,
    float  n_dot_l) {
    return (diffuse_brdf + specular_brdf) * radiance * n_dot_l;
}

float3 EvaluateDiffuseBRDF(
    float3 kD,
    float3 material_albedo) {
    return kD * material_albedo / k_PI;
}

float3 EvaluateSpecularBRDF(
    float3 kS,
    float  D,
    float3 F,
    float  G,
    float  n_dot_l,
    float  n_dot_v) {
    return kS * D * G * F / max(4 * n_dot_v * n_dot_l, 0.000001f);
}

float CalculateDistributionGGX(float n_dot_h, float roughness) {
    float a = roughness * roughness * roughness * roughness;

    float denominator = (n_dot_h * n_dot_h * (a - 1.0f) + 1.0f);

    denominator = k_PI * denominator * denominator;

    return a / denominator;
}

float CalculateGeometrySmith(float n_dot_v, float n_dot_l, float roughness) {
    float schlick_light = CalculateGeometrySchlickGGX(n_dot_l, roughness);
    float schlick_view  = CalculateGeometrySchlickGGX(n_dot_v, roughness);

    return schlick_light * schlick_view;
}

float CalculateGeometrySchlickGGX(float dot_product, float roughness) {
    float r = roughness + 1.0f;
    float k = r * r / 8.0f;

    float denominator = dot_product * (1.0f - k) + k;

    return dot_product / denominator;
}

float3 CalculateFresnelSchlick(float v_dot_h, float3 f0) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - v_dot_h, 0.0f, 1.0f), 5.0f);
}

float3 GetF0Metallic(float3 albedo, float metallic) {
    return lerp(float3(0.04f), albedo, metallic);
}

