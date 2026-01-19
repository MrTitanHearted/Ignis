#include <Ignis/Render.hpp>

namespace Ignis {

    void Render::generateBRDFLUTMap(const Settings &settings) {
        const FileAsset shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/GenerateBRDFLUTMap.spv").value();

        std::vector<uint32_t> shader_code{};
        shader_code.resize(shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(shader_code.data(), shader_file.getContent().data(), shader_file.getSize());

        const vk::ShaderModule shader_module = Vulkan::CreateShaderModuleFromSPV(shader_code);

        const uint32_t &gImageSize = settings.BRDFLUTResolution;

        m_BRDFLUTImage = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            {gImageSize, gImageSize});
        m_BRDFLUTImageView = Vulkan::CreateImageColorView2D(m_BRDFLUTImage.Handle, m_BRDFLUTImage.Format);

        const vk::PipelineLayout pipeline_layout =
            Vulkan::CreatePipelineLayout({}, {});

        const vk::Pipeline pipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", shader_module)
                .setFragmentShader("fs_main", shader_module)
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_BRDFLUTImage.Format})
                .setNoDepthTest()
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(pipeline_layout);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BeginDebugUtilsLabel(command_buffer, "Ignis::Render::GenerateBRDFLUTMap", {1.0f, 1.0f, 0.0f, 1.0f});

            Vulkan::BarrierMerger merger{};

            merger.putImageBarrier(
                m_BRDFLUTImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite);
            merger.flushBarriers(command_buffer);

            Vulkan::BeginRenderPass(
                {gImageSize, gImageSize},
                {Vulkan::GetRenderingAttachmentInfo(
                    m_BRDFLUTImageView,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f})},
                std::nullopt,
                command_buffer);
            vk::Viewport viewport{};
            viewport
                .setX(0.0f)
                .setY(static_cast<glm::f32>(gImageSize))
                .setWidth(static_cast<glm::f32>(gImageSize))
                .setHeight(-static_cast<glm::f32>(gImageSize))
                .setMinDepth(0.0f)
                .setMaxDepth(1.0f);
            vk::Rect2D scissor{};
            scissor
                .setOffset(vk::Offset2D{0, 0})
                .setExtent({gImageSize, gImageSize});
            command_buffer.setViewport(0, {viewport});
            command_buffer.setScissor(0, {scissor});

            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            command_buffer.draw(6, 1, 0, 0);

            Vulkan::EndRenderPass(command_buffer);

            merger.putImageBarrier(
                m_BRDFLUTImage.Handle,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderRead);
            merger.flushBarriers(command_buffer);

            Vulkan::EndDebugUtilsLabel(command_buffer);
        });

        Vulkan::DestroyPipeline(pipeline);
        Vulkan::DestroyPipelineLayout(pipeline_layout);
        Vulkan::DestroyShaderModule(shader_module);
    }
}  // namespace Ignis