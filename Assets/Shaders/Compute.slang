[[vk::binding(0, 0)]]
[[vk::image_format("rgba32f")]]
RWTexture2D<float4> OutputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(
    uint3 thread_id: SV_DispatchThreadID,
    uint3 group_id: SV_GroupThreadID) {
    uint2 texel_coord = thread_id.xy;

    uint2 output_image_size;
    OutputImage.GetDimensions(output_image_size.x, output_image_size.y);

    if (texel_coord.x < output_image_size.x &&
        texel_coord.y < output_image_size.y) {
        float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

        if (group_id.x != 0 && group_id.y != 0) {
            color.x = float(texel_coord.x) / (output_image_size.x);
            color.y = float(texel_coord.y) / (output_image_size.y);
        }

        OutputImage[texel_coord] = color;
        return;
    }

    OutputImage[texel_coord] = float4(0.0f, 0.0f, 0.0f, 0.0f);
}

