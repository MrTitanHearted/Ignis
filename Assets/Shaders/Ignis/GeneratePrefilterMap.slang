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

struct PushConstant {
    float Roughness;
    float SkyboxResolution;
};

[[vk::push_constant]]
PushConstant gPC;

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
    float3 R = N;
    float3 V = R;

    float3 prefilter_color = float3(0.0f);

    const uint SAMPLE_COUNT = 1024u;

    float total_weight = 0.0f;

    for (uint i = 0; i < SAMPLE_COUNT; i++) {
        float2 Xi = CalculateHammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, gPC.Roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float n_dot_l = saturate(dot(N, L));
        if (n_dot_l > 0.0f) {
            float n_dot_h = saturate(dot(N, H));
            float v_dot_h = saturate(dot(V, H));

            float D   = CalculateDistributionGGX(n_dot_h, gPC.Roughness);
            float pdf = D * n_dot_h / (4.0f * v_dot_h) + 0.0001f;

            float resolution = gPC.SkyboxResolution;

            float omega_s = 1.0f / (float(SAMPLE_COUNT) * pdf);
            float omega_p = 4.0f * k_PI / (6.0f * resolution * resolution);

            float mip_level = 0.0f == gPC.Roughness ? (0.0f) : (0.5f * log2(omega_s / omega_p));

            prefilter_color += gSkybox.SampleLevel(L, mip_level).rgb * n_dot_l;
            total_weight += n_dot_l;
        }
    }

    prefilter_color = prefilter_color / total_weight;

    return float4(prefilter_color, 1.0f);
}

float CalculateDistributionGGX(float n_dot_h, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;

    float nominator   = a2;
    float denominator = n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f;
    denominator       = k_PI * denominator * denominator;

    return nominator / denominator;
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
    float a = roughness * roughness;

    float phi       = 2.0f * k_PI * Xi.x;
    float cos_theta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    float3 H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;

    float3 up        = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sample_vector = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sample_vector);
}

float2 CalculateHammersley(uint i, uint N) {
    return float2(float(i) / float(N), CalculateRadianceInverseVdC(i));
}

float CalculateRadianceInverseVdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;  // / 0x100000000
}

