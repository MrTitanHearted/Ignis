#include <Ignis/Render.hpp>

namespace Ignis {
    void Render::generateIrradianceMap() {
        const FileAsset shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/GenerateIrradianceMap.spv").value();

        std::vector<uint32_t> shader_code{};
        shader_code.resize(shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(shader_code.data(), shader_file.getContent().data(), shader_file.getSize());

        const vk::ShaderModule shader_module = Vulkan::CreateShaderModuleFromSPV(shader_code);

        m_IrradianceImage = Vulkan::AllocateImageCube(
            {}, vma::MemoryUsage::eGpuOnly,
            vk::ImageCreateFlagBits::eCubeCompatible,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            {32, 32});
        m_IrradianceImageView = Vulkan::CreateImageColorViewCube(m_IrradianceImage.Handle, m_IrradianceImage.Format);

        const vk::DescriptorPool descriptor_pool =
            Vulkan::CreateDescriptorPool(
                {}, 1,
                {vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}});

        const vk::DescriptorSetLayout set_layout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
                .build();

        const vk::PipelineLayout pipeline_layout =
            Vulkan::CreatePipelineLayout({}, {set_layout});

        const vk::Pipeline pipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", shader_module)
                .setFragmentShader("fs_main", shader_module)
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_IrradianceImage.Format})
                .setViewMask(0b00111111)
                .setNoDepthTest()
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(pipeline_layout);

        const vk::DescriptorSet descriptor_set =
            Vulkan::AllocateDescriptorSet(set_layout, descriptor_pool);

        const vk::ImageView render_image_view =
            Vulkan::CreateImageColorView2DArray(m_IrradianceImage.Handle, m_IrradianceImage.Format, 0, 6);

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, m_SkyboxImageView, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .update(descriptor_set);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};

            merger.putImageBarrier(
                m_IrradianceImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite);
            merger.flushBarriers(command_buffer);

            Vulkan::BeginRenderPass(
                {32, 32},
                {Vulkan::GetRenderingAttachmentInfo(
                    render_image_view,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f})},
                std::nullopt,
                0b00111111,
                command_buffer);
            vk::Viewport viewport{};
            viewport
                .setX(0.0f)
                .setY(32.0f)
                .setWidth(32.0f)
                .setHeight(-32.0f)
                .setMinDepth(0.0f)
                .setMaxDepth(1.0f);
            vk::Rect2D scissor{};
            scissor
                .setOffset(vk::Offset2D{0, 0})
                .setExtent({32, 32});
            command_buffer.setViewport(0, {viewport});
            command_buffer.setScissor(0, {scissor});

            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, {descriptor_set}, {});
            command_buffer.draw(36, 1, 0, 0);

            Vulkan::EndRenderPass(command_buffer);

            merger.putImageBarrier(
                m_IrradianceImage.Handle,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderRead);
            merger.flushBarriers(command_buffer);
        });

        Vulkan::DestroyPipeline(pipeline);
        Vulkan::DestroyPipelineLayout(pipeline_layout);
        Vulkan::DestroyDescriptorSetLayout(set_layout);
        Vulkan::DestroyDescriptorPool(descriptor_pool);
        Vulkan::DestroyImageView(render_image_view);
        Vulkan::DestroyShaderModule(shader_module);
    }
}  // namespace Ignis