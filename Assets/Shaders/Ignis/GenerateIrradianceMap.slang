module Ignis;

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float3 UVW;
};

const static float k_PI = 3.14159265358979323846f;

const static float3 gCubeVertices[36] =
    {
        // +X
        float3(1, -1, -1), float3(1, 1, -1), float3(1, 1, 1),
        float3(1, -1, -1), float3(1, 1, 1), float3(1, -1, 1),

        // -X
        float3(-1, -1, 1), float3(-1, 1, 1), float3(-1, 1, -1),
        float3(-1, -1, 1), float3(-1, 1, -1), float3(-1, -1, -1),

        // +Y
        float3(-1, 1, -1), float3(-1, 1, 1), float3(1, 1, 1),
        float3(-1, 1, -1), float3(1, 1, 1), float3(1, 1, -1),

        // -Y
        float3(-1, -1, 1), float3(-1, -1, -1), float3(1, -1, -1),
        float3(-1, -1, 1), float3(1, -1, -1), float3(1, -1, 1),

        // +Z
        float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1),
        float3(-1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1),

        // -Z
        float3(1, -1, -1), float3(-1, -1, -1), float3(-1, 1, -1),
        float3(1, -1, -1), float3(-1, 1, -1), float3(1, 1, -1)
    };

[[vk::binding(0, 0)]]
SamplerCube gSkybox;

[shader("vertex")]
VertexOutput vs_main(
    uint vertex_id: SV_VertexID,
    uint face_id: SV_ViewID) {
    float3 vertex = gCubeVertices[vertex_id];
    float3 uvw    = float3(0.0f);

    switch (face_id) {
        case 0:
            uvw = float3(1, vertex.y, -vertex.x);
            break;  // +X
        case 1:
            uvw = float3(-1, vertex.y, vertex.x);
            break;  // -X
        case 2:
            uvw = float3(vertex.x, 1, -vertex.y);
            break;  // +Y
        case 3:
            uvw = float3(vertex.x, -1, vertex.y);
            break;  // -Y
        case 4:
            uvw = float3(vertex.x, vertex.y, 1);
            break;  // +Z
        case 5:
            uvw = float3(-vertex.x, vertex.y, -1);
            break;  // -Z
    }

    VertexOutput output;
    output.Position = float4(vertex, 1.0f);
    output.UVW      = normalize(uvw);
    return output;
}

[shader("fragment")]
float4 fs_main([[vk::location(0)]] float3 uvw) : SV_Target {
    float3 N = normalize(uvw);

    float3 irradiance = float3(0.0f);

    float3 up    = float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, N));

    up = normalize(cross(N, right));

    float sample_delta = 0.02f;
    float sample_count = 0.0f;

    for (float phi = 0.0f; phi < 2.0f * k_PI; phi += sample_delta) {
        for (float theta = 0.0f; theta < 0.5f * k_PI; theta += sample_delta) {
            float3 tangent_sample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sample_vector  = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N;

            irradiance += gSkybox.Sample(sample_vector).rgb * cos(theta) * sin(theta);

            sample_count += 1.0f;
        }
    }

    irradiance = k_PI * irradiance / sample_count;

    return float4(irradiance, 1.0f);
}

