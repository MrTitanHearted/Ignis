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

struct PBRState {
    float3 N;
    float3 V;

    float NoL;
    float NoV;
    float NoH;
    float LoH;
    float VoH;

    float  PerceptualRoughness;
    float  AlphaRoughness;
    float3 Reflectance0;
    float3 Reflectance90;

    float3 BaseDiffuseColor;
    float3 BaseSpecularColor;

    float3 FssEss;

    float BRDFScale;
    float BRDFBias;
};

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
Sampler2D gBRDFLUTTexture;
[[vk::binding(2, 0)]]
SamplerCube gPrefilterTexture;
[[vk::binding(3, 0)]]
SamplerCube gIrradianceTexture;
[[vk::binding(4, 0)]]
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

    float MaxPrefilterMipLevel;

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

    {
        float3 texture_albedo = gTextures[material.AlbedoTexture].Sample(input.UV).rgb;

        material_albedo *= texture_albedo;
    }
    {
        float3 tangent_normal = gTextures[material.NormalTexture].Sample(input.UV).rgb;

        tangent_normal = normalize(tangent_normal * 2.0f - 1.0f);

        float handedness = input.Tangent.w;

        float3 N = normalize(input.Normal);
        float3 T = normalize(input.Tangent.xyz);

        T = normalize(T - N * dot(N, T));

        float3 B = normalize(handedness * cross(N, T));

        float3x3 TBN = transpose(float3x3(T, B, N));

        material_normal = mul(TBN, tangent_normal);
    }
    {
        material_emission *= gTextures[material.EmissiveTexture].Sample(input.UV).rgb;
    }
    {
        material_ao *= gTextures[material.AmbientOcclusionTexture].Sample(input.UV).rgb;
    }
    {
        float2 material_metallic_roughness = gTextures[material.MetallicRoughnessTexture].Sample(input.UV).gb;

        material_metallic *= material_metallic_roughness.g;
        material_roughness *= material_metallic_roughness.r;
    }
    {
        material_metallic *= gTextures[material.MetallicTexture].Sample(input.UV).r;
        material_roughness *= gTextures[material.RoughnessTexture].Sample(input.UV).r;
    }

    // PBR lighting calculation
    float3 N = normalize(material_normal);
    float3 V = normalize(gDrawPC.ViewPosition - input.FragmentPosition);

    float n_dot_v = saturate(dot(N, V));

    float2 brdf = gBRDFLUTTexture.Sample(float2(n_dot_v, material_roughness)).rg;

    float3 f0                  = GetF0Metallic(material_albedo, material_metallic);
    float3 base_diffuse_color  = lerp(material_albedo, float3(0.0f), material_metallic);
    float3 base_specular_color = lerp(f0, material_albedo, material_metallic);

    float reflectance   = max(max(base_specular_color.r, base_specular_color.g), base_specular_color.b);
    float reflectance90 = saturate(reflectance * 25.0f);

    PBRState pbr_state;
    pbr_state.N   = N;
    pbr_state.V   = V;
    pbr_state.NoV = n_dot_v;

    pbr_state.PerceptualRoughness = material_roughness;
    pbr_state.AlphaRoughness      = material_roughness * material_roughness;
    pbr_state.Reflectance0        = base_specular_color;
    pbr_state.Reflectance90       = reflectance90;
    pbr_state.BaseDiffuseColor    = base_diffuse_color;
    pbr_state.BaseSpecularColor   = base_specular_color;

    pbr_state.BRDFScale = brdf.x;
    pbr_state.BRDFBias  = brdf.y;

    float3 fr  = max(float3(1.0f - pbr_state.PerceptualRoughness), pbr_state.Reflectance0) - pbr_state.Reflectance0;
    float3 k_s = pbr_state.Reflectance0 + fr * pow(1.0f - pbr_state.NoV, 5.0f);

    pbr_state.FssEss = k_s * pbr_state.BRDFScale + pbr_state.BRDFBias;

    float3 result = float3(0.0f);

    // Directional light
    {
        float3 L = normalize(-gDirectionLight.Direction);
        float3 H = normalize(V + L);

        float3 radiance = gDirectionLight.Color * gDirectionLight.Power;

        pbr_state.NoL = saturate(dot(N, L));
        pbr_state.NoH = saturate(dot(N, H));
        pbr_state.VoH = saturate(dot(V, H));
        pbr_state.LoH = saturate(dot(L, H));

        float  D = CalculateDistributionTrowbrigeReitz(&pbr_state);
        float3 F = CalculateFresnelSchlick(&pbr_state);
        float  G = CalculateGeometricOcclusion(&pbr_state);

        float3 kS = F;
        float3 kD = (1.0f - kS);

        float3 diffuse_brdf  = EvaluateDiffuseBRDFDisneyBurley(kD, &pbr_state);
        float3 specular_brdf = EvaluateSpecularBRDF(D, F, G, pbr_state.NoL, pbr_state.NoV);

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, pbr_state.NoL);
    }

    for (uint i = 0; i < gLightData.PointLightCount; i++) {
        float3 LightToFragment = gPointLights[i].Position - input.FragmentPosition;

        float3 L = normalize(LightToFragment);
        float3 H = normalize(V + L);

        float distance    = length(LightToFragment);
        float attenuation = 1.0f / (distance * distance);

        float3 radiance = gPointLights[i].Color * gPointLights[i].Power * attenuation;

        pbr_state.NoL = saturate(dot(N, L));
        pbr_state.NoH = saturate(dot(N, H));
        pbr_state.VoH = saturate(dot(V, H));
        pbr_state.LoH = saturate(dot(L, H));

        float  D = CalculateDistributionTrowbrigeReitz(&pbr_state);
        float3 F = CalculateFresnelSchlick(&pbr_state);
        float  G = CalculateGeometricOcclusion(&pbr_state);

        float3 kS = F;
        float3 kD = (1.0f - kS);

        float3 diffuse_brdf  = EvaluateDiffuseBRDFDisneyBurley(kD, &pbr_state);
        float3 specular_brdf = EvaluateSpecularBRDF(D, F, G, pbr_state.NoL, pbr_state.NoV);

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, pbr_state.NoL);
    }

    for (uint i = 0; i < gLightData.SpotLightCount; i++) {
        float3 LightToFragment = gSpotLights[i].Position - input.FragmentPosition;

        float3 L = normalize(LightToFragment);
        float3 H = normalize(V + L);

        float theta     = dot(L, normalize(-gSpotLights[i].Direction));
        float epsilon   = gSpotLights[i].CutOff - gSpotLights[i].OuterCutOff;
        float intensity = saturate((theta - gSpotLights[i].OuterCutOff) / epsilon);

        float distance    = length(LightToFragment);
        float attenuation = 1.0f / (distance * distance);

        float3 radiance = gSpotLights[i].Color * gSpotLights[i].Power * attenuation * intensity;

        pbr_state.NoL = saturate(dot(N, L));
        pbr_state.NoH = saturate(dot(N, H));
        pbr_state.VoH = saturate(dot(V, H));
        pbr_state.LoH = saturate(dot(L, H));

        float  D = CalculateDistributionTrowbrigeReitz(&pbr_state);
        float3 F = CalculateFresnelSchlick(&pbr_state);
        float  G = CalculateGeometricOcclusion(&pbr_state);

        float3 kS = F;
        float3 kD = (1.0f - kS);

        float3 diffuse_brdf  = EvaluateDiffuseBRDFDisneyBurley(kD, &pbr_state);
        float3 specular_brdf = EvaluateSpecularBRDF(D, F, G, pbr_state.NoL, pbr_state.NoV);

        result += EvaluateBRDF(diffuse_brdf, specular_brdf, radiance, pbr_state.NoL);
    }

    // IBL
    {
        float3 R = normalize(reflect(-pbr_state.V, pbr_state.N));

        float3 radiance =
            gPrefilterTexture.SampleLevel(R, pbr_state.PerceptualRoughness * gDrawPC.MaxPrefilterMipLevel).rgb;
        float3 specular_ibl = radiance * pbr_state.FssEss;

        float3 irradiance  = gIrradianceTexture.Sample(pbr_state.N).rgb;
        float  ems         = 1.0f - (pbr_state.BRDFScale + pbr_state.BRDFBias);
        float3 f_average   = pbr_state.Reflectance0 + (1.0f - pbr_state.Reflectance0) / 21.0f;
        float3 fms_ems     = f_average * ems * pbr_state.FssEss / (1.0f - f_average * ems);
        float3 k_d         = pbr_state.BaseDiffuseColor * (1.0f - pbr_state.FssEss + fms_ems);
        float3 diffuse_ibl = (fms_ems + k_d) * irradiance;

        float3 ibl = diffuse_ibl + specular_ibl;

        result += ibl;
    }

    result += material_emission;

    result = ApplyToneMappingACES(result);
    result = ApplyGammaCorrection(result, 2.2f);

    return float4(result, 1.0f);
}

float3 EvaluateBRDF(
    float3 diffuse_brdf,
    float3 specular_brdf,
    float3 radiance,
    float  n_dot_l) {
    return (diffuse_brdf + specular_brdf) * radiance * n_dot_l;
}

float3 EvaluateDiffuseBRDFDisneyBurley(
    float3    k_D,
    PBRState *state) {
    float f90 = 2.0f * state->LoH * state->LoH * state->AlphaRoughness - 0.5f;

    return k_D * (state->BaseDiffuseColor / k_PI) *
           (1.0f + f90 * pow(1.0f - state->NoL, 5.0f)) *
           (1.0f + f90 * pow(1.0f - state->NoV, 5.0f));
}

float3 EvaluateDiffuseBRDF(
    float3 kD,
    float3 material_albedo) {
    return kD * material_albedo / k_PI;
}

float3 EvaluateSpecularBRDF(
    float  D,
    float3 F,
    float  G,
    float  n_dot_l,
    float  n_dot_v) {
    return D * G * F / (4 * n_dot_v * n_dot_l + 0.001f);
}

float CalculateDistributionTrowbrigeReitz(PBRState *state) {
    float roughness_sq = state->AlphaRoughness * state->AlphaRoughness;

    float f = (state->NoH * roughness_sq - state->NoH) * state->NoH + 1.0f;

    return roughness_sq / (k_PI * f * f);
}

float CalculateDistributionGGX(float n_dot_h, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;

    float nominator   = a2;
    float denominator = (n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f);

    denominator = k_PI * denominator * denominator;

    return nominator / denominator;
}

float CalculateGeometricOcclusion(PBRState *state) {
    float n_dot_l = state->NoL;
    float n_dot_v = state->NoV;

    float r_2 = state->AlphaRoughness * state->AlphaRoughness;

    float attenuation_l = 2.0f * n_dot_l / (n_dot_l + sqrt(r_2 + (1.0f - r_2) * n_dot_l * n_dot_v));
    float attenuation_v = 2.0f * n_dot_v / (n_dot_v + sqrt(r_2 + (1.0f - r_2) * n_dot_v * n_dot_l));

    return attenuation_l * attenuation_v;
}

float CalculateGeometrySmith(float n_dot_v, float n_dot_l, float roughness) {
    float schlick_view  = CalculateGeometrySchlickGGX(n_dot_v, roughness);
    float schlick_light = CalculateGeometrySchlickGGX(n_dot_l, roughness);

    return schlick_view * schlick_light;
}

float CalculateGeometrySchlickGGX(float dot_product, float roughness) {
    float r = roughness + 1.0f;
    float k = r * r / 8.0f;

    float nominator   = dot_product;
    float denominator = dot_product * (1.0f - k) + k;

    return nominator / denominator;
}

float3 CalculateFresnelSchlick(PBRState *state) {
    return state->Reflectance0 +
           (state->Reflectance90 - state->Reflectance0) *
               pow(saturate(1.0f - state->VoH), 5.0f);
}

float3 CalculateFresnelSchlick(float a_dot_b, float3 f0) {
    return f0 + (1.0f - f0) * pow(1.0f - a_dot_b, 5.0f);
}

float3 CalculateFresnelSchlickRoughness(float a_dot_b, float3 f0, float roughness) {
    return f0 + (max(float3(1.0f - roughness), f0) - f0) * pow(1.0f - a_dot_b, 5.0f);
}

float3 CalculateFresnelDotpoint7(float a_dot_b, float3 f0) {
    return f0 + (1.0f - a_dot_b - f0) * pow(1.0f - a_dot_b, 4.0f);
}

float3 GetF0Metallic(float3 albedo, float metallic) {
    return lerp(float3(0.04f), albedo, metallic);
}

float3 ApplyGammaCorrection(float3 color, float gamma) {
    return pow(color, float3(1.0f / gamma));
}

float3 ApplyToneMappingHDR(float3 color) {
    return color / (color + 1.0f);
}

float3 ApplyToneMappingACES(float3 color) {
    const float3x3 m1 = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777);

    const float3x3 m2 = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602);

    float3 v = mul(m1, color);
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return saturate(mul(m2, a / b));
}

