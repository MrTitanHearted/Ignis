struct Vertex {
    float4 Position;
    float4 Color;
};

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float4 Color;
};

struct FragmentInput {
    [[vk::location(0)]]
    float4 Color;
};

struct PCVertexInput {
    Vertex *pVertices;
};

[[vk::push_constant]]
ConstantBuffer<PCVertexInput> PushConstant;

[shader("vertex")]
VertexOutput vertexMain(uint vertex_id: SV_VertexID) {
    Vertex v = PushConstant.pVertices[vertex_id];

    VertexOutput output;

    output.Position = v.Position;
    output.Color    = v.Color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentInput input) : SV_Target {
    return input.Color;
}

