[[vk::binding(0, 0)]]
[[vk::image_format("rgba32f")]]
RWTexture2D<float4> OutputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 thread_id: SV_DispatchThreadID) {
    uint2 texel_coord        = thread_id.xy;
    OutputImage[texel_coord] = float4(1.0f, 0.0f, 0.0f, 1.0f);
}

