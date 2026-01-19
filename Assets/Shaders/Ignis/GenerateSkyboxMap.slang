module Ignis;

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float3 Direction;
};

const static float k_PI = 3.14159265358979323846f;

const static float2 gPositions[4] = {
    float2(-1.0f, 1.0f),
    float2(-1.0f, -1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, -1.0f),
};

const static uint gIndices[6] = {
    1, 0, 2,
    1, 2, 3
};

[[vk::binding(0, 0)]]
Sampler2D gTexture;

[shader("vertex")]
VertexOutput vs_main(uint vertex_id: SV_VertexID, uint face_id: SV_ViewID) {
    uint32_t index  = gIndices[vertex_id];
    float2   vertex = gPositions[index];

    float3 direction;
    switch (face_id) {
        case 0:
            direction = float3(1, vertex.y, -vertex.x);
            break;  // +X
        case 1:
            direction = float3(-1, vertex.y, vertex.x);
            break;  // -X
        case 2:
            direction = float3(vertex.x, 1, -vertex.y);
            break;  // +Y
        case 3:
            direction = float3(vertex.x, -1, vertex.y);
            break;  // -Y
        case 4:
            direction = float3(vertex.x, vertex.y, 1);
            break;  // +Z
        case 5:
            direction = float3(-vertex.x, vertex.y, -1);
            break;  // -Z
    }

    VertexOutput output;

    output.Position  = float4(vertex, 0.0f, 1.0f);
    output.Direction = direction;

    return output;
}

[shader("fragment")]
float4 fs_main([[vk::location(0)]] float3 direction) : SV_Target {
    direction = normalize(direction);

    float2 uv = MapDirectionToUV(direction);

    float3 result = gTexture.Sample(uv).rgb;
    return float4(result, 1.0f);
}

float2 MapDirectionToUV(float3 direction) {
    float u = atan2(direction.z, direction.x) * (1.0f / (2.0f * k_PI)) + 0.5f;
    float v = 1.0f - asin(direction.y) * (1.0f / k_PI) + 0.5f;
    return float2(u, v);
}

