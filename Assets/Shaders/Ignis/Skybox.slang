module Ignis;

struct Vertex {
    [[vk::location(0)]]
    float3 Position;
};

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float3 UVW;
};

typedef VertexOutput FragmentInput;

[[vk::binding(0, 0)]]
SamplerCube gSkybox;

struct Camera {
    float4x4 ProjectionView;
};

[[vk::push_constant]]
ConstantBuffer<Camera> gCamera;

[shader("vertex")]
VertexOutput vs_main(
    Vertex input) {
    VertexOutput output;

    output.Position = mul(gCamera.ProjectionView, float4(input.Position, 1.0f));

    output.Position.z = output.Position.w;

    output.UVW = input.Position;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    float3 color = gSkybox.Sample(input.UVW).rgb;

    // color = ApplyToneMappingACES(color);
    // color = ApplyGammaCorrection(color, 2.2f);

    return float4(color, 1.0f);
}

float3 ApplyGammaCorrection(float3 color, float gamma) {
    return pow(color, float3(1.0f / gamma));
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
