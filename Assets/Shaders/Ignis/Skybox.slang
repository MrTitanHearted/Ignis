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

    return float4(color, 1.0f);
}

