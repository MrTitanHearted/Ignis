struct Vertex {
    [vk::location(0)]
    float3 Position;
    [vk::location(1)]
    float3 Color;
};

struct VertexOutput {
    [vk::location(0)]
    float4 Position : SV_Position;
    [vk::location(1)]
    float3 Color;
};

struct FragmentInput {
    [vk::location(0)]
    float3 Position;
    [vk::location(1)]
    float3 Color;
};

struct Camera {
    column_major float4x4 Projection;
    column_major float4x4 View;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> gCamera;

[shader("vertex")]
VertexOutput vs_main(Vertex v) {
    VertexOutput output;

    output.Position = mul(gCamera.Projection, mul(gCamera.View, float4(v.Position, 1.0f)));
    output.Color    = v.Color;

    return output;
}

[shader("fragment")]
float4 fs_main(FragmentInput input) : SV_Target {
    return float4(input.Color, 1.0f);
}

