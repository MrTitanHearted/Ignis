module Ignis;

struct VertexOutput {
    float4 Position : SV_Position;
    [[vk::location(0)]]
    float2 UV;
};

const static float k_PI = 3.14159265358979323846f;

const static float2 gPositions[4] = {
    float2(-1.0f, 1.0f),
    float2(-1.0f, -1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, -1.0f),
};

const static float2 gUVs[4] = {
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, 0.0f),
};

const static uint gIndices[6] = {
    1, 0, 2,
    1, 2, 3
};

[shader("vertex")]
VertexOutput vs_main(uint vertex_id: SV_VertexID) {
    uint32_t index = gIndices[vertex_id];

    VertexOutput output;

    output.Position = float4(gPositions[index], 0.0f, 1.0f);
    output.UV       = gUVs[index];

    return output;
}

[shader("fragment")]
float2 fs_main([[vk::location(0)]] float2 uv) : SV_Target {
    return IntegrateBRDF(uv.x, uv.y);
}

float2 IntegrateBRDF(float n_dot_v, float roughness) {
    float3 V;
    V.x = sqrt(1.0f - n_dot_v * n_dot_v);
    V.y = 0.0f;
    V.z = n_dot_v;

    float A = 0.0f;
    float B = 0.0f;

    float3 N = float3(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 1024u;

    for (uint i = 0; i < SAMPLE_COUNT; i++) {
        float2 Xi = CalculateHammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float n_dot_l = saturate(L.z);
        float n_dot_h = saturate(H.z);
        float v_dot_h = saturate(dot(V, H));

        if (n_dot_l > 0.0f) {
            float G = CalculateGeometrySmith(n_dot_l, n_dot_v, roughness) * v_dot_h / (n_dot_h * n_dot_v);
            float Fc = pow(1.0f - v_dot_h, 5.0f);

            A += (1.0f - Fc) * G;
            B += Fc * G;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return float2(A, B);
}

float CalculateGeometryVSmithGGXCorrelated(float n_dot_l, float n_dot_v, float roughness) {
    float a2   = roughness * roughness * roughness * roughness;
    float ggxl = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0f - a2) + a2);
    float ggxv = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0f - a2) + a2);
    return 0.5f / (ggxl + ggxv);
}

float CalculateGeometrySmith(float n_dot_l, float n_dot_v, float roughness) {
    float schlick_light = CalculateGeometrySchlickGGX(n_dot_l, roughness);
    float schlick_view  = CalculateGeometrySchlickGGX(n_dot_v, roughness);

    return schlick_light * schlick_view;
}

float CalculateGeometrySchlickGGX(float dot_product, float roughness) {
    float r = roughness;
    float k = r * r / 2.0f;

    float nominator   = dot_product;
    float denominator = dot_product * (1.0f - k) + k;

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
    return float(bits) * 2.3283064365386963e-10;
}

