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
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float3 FragmentPosition;
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

struct Camera {
    column_major float4x4 Projection;
    column_major float4x4 View;
};

struct DirectionalLight {
    float3 Direction;

    float3 Ambient;
    float3 Diffuse;
    float3 Specular;
};

struct PointLight {
    float3 Position;

    float Constant;
    float Linear;
    float Quadratic;

    float3 Ambient;
    float3 Diffuse;
    float3 Specular;
};

struct SpotLight {
    float3 Position;
    float3 Direction;

    float CutOff;
    float OuterCutOff;
    float _ignis_padding;

    float Constant;
    float Linear;
    float Quadratic;

    float3 Ambient;
    float3 Diffuse;
    float3 Specular;
};

struct LightData {
    uint PointLightCount;
    uint SpotLightCount;
    uint _ignis_padding;

    float3 ViewPosition;
};

struct Instance {
    column_major float4x4 ModelTransform;
    column_major float4x4 NormalTransform;
};

[[vk::binding(0, 0)]]
Sampler2D gTextures[];
[[vk::binding(1, 0)]]
StructuredBuffer<Material> gMaterials;
[[vk::binding(2, 0)]]
ConstantBuffer<Camera> gCamera;

[[vk::binding(0, 1)]]
ConstantBuffer<DirectionalLight> gDirectionLight;
[[vk::binding(1, 1)]]
StructuredBuffer<PointLight> gPointLights;
[[vk::binding(2, 1)]]
StructuredBuffer<SpotLight> gSpotLights;
[[vk::binding(3, 1)]]
ConstantBuffer<LightData> gLightData;

[[vk::binding(0, 2)]]
StructuredBuffer<Instance> gInstances[];

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

    output.FragmentPosition = mul(model, float4(input.Position, 1.0f)).xyz;

    output.Normal = mul(float3x3(normal), input.Normal);
    output.UV     = input.UV;

    output.Material = gDrawData.Material;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    Material material = gMaterials[input.Material];

    float3 normal = normalize(input.Normal);

    float3 view_direction = normalize(gLightData.ViewPosition - input.FragmentPosition);

    float3 material_diffuse  = gTextures[material.DiffuseTexture].Sample(input.UV).rgb;
    float3 material_specular = gTextures[material.SpecularTexture].Sample(input.UV).rgb;

    float shininess = material.Shininess;

    float3 result = CalculateDirectionLight(
        gDirectionLight,
        normal,
        view_direction,
        shininess,
        material_diffuse,
        material_specular);

    for (uint i = 0; i < gLightData.PointLightCount; i++) {
        result += CalculatePointLight(
            gPointLights.Load(i),
            normal,
            input.FragmentPosition,
            view_direction,
            shininess,
            material_diffuse,
            material_specular);
    }

    for (uint i = 0; i < gLightData.SpotLightCount; i++) {
        result += CalculateSpotLight(
            gSpotLights.Load(i),
            normal,
            input.FragmentPosition,
            view_direction,
            shininess,
            material_diffuse,
            material_specular);
    }

    return float4(result, 1.0f);
}

float3 CalculateDirectionLight(
    DirectionalLight light,

    float3 normal,
    float3 view_direction,
    float  shininess,
    float3 material_diffuse,
    float3 material_specular) {
    float3 light_direction = normalize(-light.Direction);

    float diffuse_power = max(dot(normal, light_direction), 0.0f);

    float3 reflect_direction = reflect(-light_direction, normal);
    float  specular_power    = pow(max(dot(view_direction, reflect_direction), 0.0f), shininess);

    float3 ambient  = light.Ambient * material_diffuse;
    float3 diffuse  = light.Diffuse * diffuse_power * material_diffuse;
    float3 specular = light.Specular * specular_power * material_specular;
    return ambient + diffuse + specular;
}

float3 CalculatePointLight(
    PointLight light,
    float3     normal,
    float3     fragment_position,
    float3     view_direction,
    float      shininess,
    float3     material_diffuse,
    float3     material_specular) {
    float3 light_direction = normalize(light.Position - fragment_position);

    float diffuse_power = max(dot(normal, light_direction), 0.0f);

    float3 reflect_direction = reflect(-light_direction, normal);
    float  specular_power    = pow(max(dot(view_direction, light_direction), 0.0f), shininess);

    float distance    = length(light.Position - fragment_position);
    float attenuation = 1.0f / (light.Constant + light.Linear * distance + light.Quadratic * distance * distance);

    float3 ambient  = light.Ambient * material_diffuse;
    float3 diffuse  = light.Diffuse * diffuse_power * material_diffuse;
    float3 specular = light.Specular * specular_power * material_specular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return ambient + diffuse + specular;
}

float3 CalculateSpotLight(
    SpotLight light,
    float3    normal,
    float3    fragment_position,
    float3    view_direction,
    float     shininess,
    float3    material_diffuse,
    float3    material_specular) {
    float3 light_direction = normalize(light.Position - fragment_position);

    float diffuse_power = max(dot(normal, light_direction), 0.0f);

    float3 reflect_direction = reflect(-light_direction, normal);
    float  specular_power    = pow(max(dot(view_direction, light_direction), 0.0f), shininess);

    float distance    = length(light.Position - fragment_position);
    float attenuation = 1.0f / (light.Constant + light.Linear * distance + light.Quadratic * distance * distance);

    float theta     = dot(light_direction, normalize(-light.Direction));
    float epsilon   = light.CutOff - light.OuterCutOff;
    float intensity = clamp((theta - light.OuterCutOff) / epsilon, 0.0f, 1.0f);

    float3 ambient  = light.Ambient * material_diffuse;
    float3 diffuse  = light.Diffuse * diffuse_power * material_diffuse;
    float3 specular = light.Specular * specular_power * material_specular;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return ambient + diffuse + specular;
}

