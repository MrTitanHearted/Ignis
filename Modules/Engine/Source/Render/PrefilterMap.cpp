#include <Ignis/Render.hpp>

namespace Ignis {
    void Render::generatePrefilterMap(const Settings &settings) {
        const FileAsset shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/GeneratePrefilterMap.spv").value();

        std::vector<uint32_t> shader_code{};
        shader_code.resize(shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(shader_code.data(), shader_file.getContent().data(), shader_file.getSize());

        const vk::ShaderModule shader_module = Vulkan::CreateShaderModuleFromSPV(shader_code);

        const uint32_t &gImageSize = settings.PrefilterResolution;

        m_PrefilterImage = Vulkan::AllocateImageCubeWithMipLevels(
            {}, vma::MemoryUsage::eGpuOnly,
            vk::ImageCreateFlagBits::eCubeCompatible,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            {gImageSize, gImageSize});

        m_PrefilterImageView = Vulkan::CreateImageColorViewCube(
            m_PrefilterImage.Handle,
            m_PrefilterImage.Format,
            0, m_PrefilterImage.MipLevelCount);

        const vk::DescriptorPool descriptor_pool =
            Vulkan::CreateDescriptorPool(
                {}, 1,
                {vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}});

        const vk::DescriptorSetLayout set_layout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
                .build();

        const vk::PipelineLayout pipeline_layout = Vulkan::CreatePipelineLayout(
            {vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::vec2)}},
            {set_layout});

        const vk::Pipeline pipeline =
            Vulkan::GraphicsPipelineBuilder()
                .setVertexShader("vs_main", shader_module)
                .setFragmentShader("fs_main", shader_module)
                .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                .setColorAttachmentFormats({m_PrefilterImage.Format})
                .setViewMask(0b00111111)
                .setNoDepthTest()
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(pipeline_layout);

        const vk::DescriptorSet descriptor_set =
            Vulkan::AllocateDescriptorSet(set_layout, descriptor_pool);

        std::vector<vk::ImageView> render_image_views{};
        render_image_views.resize(m_PrefilterImage.MipLevelCount);

        for (uint32_t i = 0; i < m_PrefilterImage.MipLevelCount; i++) {
            render_image_views[i] = Vulkan::CreateImageColorView2DArray(
                m_PrefilterImage.Handle,
                m_PrefilterImage.Format,
                i, 1,
                0, 6);
        }

        const vk::Sampler sampler = Vulkan::CreateSampler(
            vk::SamplerCreateInfo()
                .setMinLod(0.0f)
                .setMaxLod(vk::LodClampNone)
                .setMinFilter(vk::Filter::eLinear)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setAnisotropyEnable(vk::True)
                .setMaxAnisotropy(16.0f));

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, m_SkyboxImageView, vk::ImageLayout::eShaderReadOnlyOptimal, sampler)
            .update(descriptor_set);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BeginDebugUtilsLabel(command_buffer, "Ignis::Render::GeneratePrefilterMap", {1.0f, 1.0f, 0.0f, 1.0f});

            Vulkan::BarrierMerger merger{};

            merger.putImageBarrier(
                m_PrefilterImage.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite);
            merger.flushBarriers(command_buffer);

            for (uint32_t i = 0; i < m_PrefilterImage.MipLevelCount; i++) {
                const auto &render_image_view = render_image_views[i];

                const auto mip_size = static_cast<uint32_t>(gImageSize * glm::pow(0.5f, i));

                const auto roughness = static_cast<glm::f32>(i) / (static_cast<glm::f32>(m_PrefilterImage.MipLevelCount) - 1.0f);

                const glm::vec2 pc{roughness, settings.SkyboxResolution};

                Vulkan::BeginRenderPass(
                    {mip_size, mip_size},
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
                    .setY(static_cast<glm::f32>(mip_size))
                    .setWidth(static_cast<glm::f32>(mip_size))
                    .setHeight(-static_cast<glm::f32>(mip_size))
                    .setMinDepth(0.0f)
                    .setMaxDepth(1.0f);
                vk::Rect2D scissor{};
                scissor
                    .setOffset(vk::Offset2D{0, 0})
                    .setExtent({mip_size, mip_size});
                command_buffer.setViewport(0, {viewport});
                command_buffer.setScissor(0, {scissor});

                command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
                command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, {descriptor_set}, {});

                command_buffer.pushConstants(
                    pipeline_layout,
                    vk::ShaderStageFlagBits::eFragment,
                    0,
                    sizeof(glm::vec2),
                    &pc);

                command_buffer.draw(36, 1, 0, 0);

                Vulkan::EndRenderPass(command_buffer);
            }

            merger.putImageBarrier(
                m_PrefilterImage.Handle,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderRead);
            merger.flushBarriers(command_buffer);

            Vulkan::EndDebugUtilsLabel(command_buffer);
        });

        Vulkan::DestroySampler(sampler);
        Vulkan::DestroyPipeline(pipeline);
        Vulkan::DestroyPipelineLayout(pipeline_layout);
        Vulkan::DestroyDescriptorSetLayout(set_layout);
        Vulkan::DestroyDescriptorPool(descriptor_pool);

        for (const auto &view : render_image_views) {
            Vulkan::DestroyImageView(view);
        }

        Vulkan::DestroyShaderModule(shader_module);
    }
}  // namespace Ignis