struct Vertex {
    [vk::location(0)]
    float3 Position;
    [vk::location(1)]
    float3 Color;
};

struct VertexOutput {
    float4 Position : SV_Position;
    [vk::location(0)]
    float3 Color;
};

struct FragmentInput {
    [vk::location(0)]
    float3 Color;
};

[shader("vertex")]
VertexOutput vsmain(Vertex v) {
    VertexOutput output;

    output.Position = float4(v.Position, 1.0f);
    output.Color    = v.Color;

    return output;
}

[shader("fragment")]
float4 fsmain(FragmentInput input) : SV_Target {
    return float4(input.Color, 1.0f);
}

