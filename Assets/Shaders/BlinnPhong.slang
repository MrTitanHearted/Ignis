module Ignis;

struct Vertex {
    float3 Position;
    float3 Normal;
    float2 UV;
};

struct VertexOutput {
    [[vk::location(0)]]
    float4 Position : SV_Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
};

struct FragmentInput {
    [[vk::location(0)]]
    float4 Position;
    [[vk::location(1)]]
    float3 Normal;
    [[vk::location(2)]]
    float2 UV;
};

struct SampledTexture {
    uint TextureID;
    uint SamplerID;
};

struct Camera {
    float4x4 Projection;
    float4x4 View;
};

struct Mesh {
    uint VertexBufferID;
    uint IndexBufferID;

    uint MaterialID;
};

struct Material {
    SampledTexture DiffuseTexture;
    SampledTexture SpecularTexture;

    float Shininess;
};

[[vk::binding(0, 0)]]
Texture2D gTextures[];
[[vk::binding(1, 0)]]
SamplerState gSamplers[];
[[vk::binding(2, 0)]]
StructuredBuffer<Vertex> gVertexBuffers[];
[[vk::binding(3, 0)]]
StructuredBuffer<uint> gIndexBuffers[];
[[vk::binding(4, 0)]]
ConstantBuffer<Material> gMaterials[];
[[vk::binding(5, 0)]]
ConstantBuffer<Mesh> gMeshes[];
[[vk::binding(6, 0)]]
ConstantBuffer<Camera> gCameras[];

struct DrawData {
    uint CameraID;
    uint MeshID;

    uint __padding[2];

    float4x4 Model;
};

[[vk::push_constant]]
DrawData gDrawData;

[shader("vertex")]
VertexOutput vsmain(uint draw_index: SV_VertexID) {
    VertexOutput output;

    Camera camera = gCameras[gDrawData.CameraID];
    Mesh   mesh   = gMeshes[gDrawData.MeshID];

    uint   index  = gIndexBuffers[mesh.IndexBufferID].Load(draw_index);
    Vertex vertex = gVertexBuffers[mesh.VertexBufferID].Load(index);

    output.Position = mul(camera.Projection, mul(camera.View, mul(gDrawData.Model, float4(vertex.Position, 1.0f))));
    output.Normal   = vertex.Normal;
    output.UV       = vertex.UV;

    return output;
}

[shader("fragment")]
float4 fsmain(FragmentInput input) : SV_Target {
    return float4(0.0f, 1.0f, 1.0f, 1.0f);
}

