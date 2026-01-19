#include <Ignis/Render.hpp>

namespace Ignis {

    void Render::generateSkyboxMap(const Settings &settings) {
        const std::filesystem::path &skybox_path = settings.SkyboxPath;

        const FileAsset shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/GenerateSkyboxMap.spv").value();

        std::vector<uint32_t> shader_code{};
        shader_code.resize(shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(shader_code.data(), shader_file.getContent().data(), shader_file.getSize());

        const uint32_t &gImageSize = settings.SkyboxResolution;

        const vk::ShaderModule shader_module = Vulkan::CreateShaderModuleFromSPV(shader_code);

        m_SkyboxImage = Vulkan::AllocateImageCubeWithMipLevels(
            {}, vma::MemoryUsage::eGpuOnly,
            vk::ImageCreateFlagBits::eCubeCompatible,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eSampled,
            {gImageSize, gImageSize});
        m_SkyboxImageView = Vulkan::CreateImageColorViewCube(
            m_SkyboxImage.Handle,
            m_SkyboxImage.Format,
            0,
            m_SkyboxImage.MipLevelCount);

        m_SkyboxVertexBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(glm::vec3) * 8,
            vk::BufferUsageFlagBits::eVertexBuffer |
                vk::BufferUsageFlagBits::eTransferDst);
        m_SkyboxIndexBuffer = Vulkan::AllocateBuffer(
            {}, vma::MemoryUsage::eGpuOnly, {},
            sizeof(uint16_t) * 36,
            vk::BufferUsageFlagBits::eIndexBuffer |
                vk::BufferUsageFlagBits::eTransferDst);

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
                .setColorAttachmentFormats({m_SkyboxImage.Format})
                .setViewMask(0b00111111)
                .setNoDepthTest()
                .setNoStencilTest()
                .setNoMultisampling()
                .setNoBlending()
                .build(pipeline_layout);

        const vk::DescriptorSet descriptor_set =
            Vulkan::AllocateDescriptorSet(set_layout, descriptor_pool);

        const vk::ImageView render_image_view =
            Vulkan::CreateImageColorView2DArray(m_SkyboxImage.Handle, m_SkyboxImage.Format, 0, 6);

        const TextureAsset texture_asset = TextureAsset::LoadFromPath(skybox_path, TextureAsset::Type::eRGBA32f).value();

        const uint64_t texture_asset_size = texture_asset.getData().size();

        const Vulkan::Image skybox_image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {texture_asset.getWidth(), texture_asset.getHeight()});

        const vk::ImageView skybox_image_view = Vulkan::CreateImageColorView2D(skybox_image.Handle, skybox_image.Format);

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, skybox_image_view, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .update(descriptor_set);

        const Vulkan::Buffer staging = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            texture_asset_size +
                m_SkyboxVertexBuffer.Size +
                m_SkyboxIndexBuffer.Size,
            vk::BufferUsageFlagBits::eTransferSrc);

        constexpr glm::vec3 vertices[]{
            glm::vec3{-1.0f, -1.0f, -1.0f},  // 0
            glm::vec3{1.0f, -1.0f, -1.0f},   // 1
            glm::vec3{1.0f, 1.0f, -1.0f},    // 2
            glm::vec3{-1.0f, 1.0f, -1.0f},   // 3
            glm::vec3{-1.0f, -1.0f, 1.0f},   // 4
            glm::vec3{1.0f, -1.0f, 1.0f},    // 5
            glm::vec3{1.0f, 1.0f, 1.0f},     // 6
            glm::vec3{-1.0f, 1.0f, 1.0f},    // 7
        };

        constexpr uint16_t indices[36] = {
            // +X face
            1, 5, 6,
            6, 2, 1,

            // -X face
            4, 0, 3,
            3, 7, 4,

            // +Y face
            3, 2, 6,
            6, 7, 3,

            // -Y face
            4, 5, 1,
            1, 0, 4,

            // +Z face
            5, 4, 7,
            7, 6, 5,

            // -Z face
            0, 1, 2,
            2, 3, 0};

        {
            uint64_t offset = 0;
            Vulkan::CopyMemoryToAllocation(texture_asset.getData().data(), staging.Allocation, offset, texture_asset_size);
            offset += texture_asset_size;
            Vulkan::CopyMemoryToAllocation(vertices, staging.Allocation, offset, m_SkyboxVertexBuffer.Size);
            offset += m_SkyboxVertexBuffer.Size;
            Vulkan::CopyMemoryToAllocation(indices, staging.Allocation, offset, m_SkyboxIndexBuffer.Size);
            offset += m_SkyboxIndexBuffer.Size;
        }

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BeginDebugUtilsLabel(command_buffer, "Ignis::Render::GenerateSkyboxMap", {1.0f, 1.0f, 0.0f, 1.0f});

            Vulkan::BarrierMerger merger{};

            merger.putImageBarrier(
                skybox_image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.flushBarriers(command_buffer);

            uint64_t src_offset = 0;
            Vulkan::CopyBufferToImage(
                staging.Handle,
                skybox_image.Handle,
                src_offset,
                {0, 0, 0},
                {0, 0},
                skybox_image.Extent,
                command_buffer);
            src_offset += texture_asset_size;
            Vulkan::CopyBufferToBuffer(
                staging.Handle,
                m_SkyboxVertexBuffer.Handle,
                src_offset, 0,
                m_SkyboxVertexBuffer.Size,
                command_buffer);
            src_offset += m_SkyboxVertexBuffer.Size;
            Vulkan::CopyBufferToBuffer(
                staging.Handle,
                m_SkyboxIndexBuffer.Handle,
                src_offset, 0,
                m_SkyboxIndexBuffer.Size,
                command_buffer);
            src_offset += m_SkyboxIndexBuffer.Size;

            merger.putImageBarrier(
                skybox_image.Handle,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.flushBarriers(command_buffer);

            merger.putImageBarrier(
                m_SkyboxImage.Handle,
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
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, {descriptor_set}, {});
            command_buffer.draw(36, 1, 0, 0);

            Vulkan::EndRenderPass(command_buffer);

            Vulkan::GenerateImageCubeMipLevels(
                m_SkyboxImage.Handle,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                m_SkyboxImage.MipLevelCount,
                {m_SkyboxImage.Extent.width, m_SkyboxImage.Extent.width},
                command_buffer);

            Vulkan::EndDebugUtilsLabel(command_buffer);
        });

        Vulkan::DestroyBuffer(staging);
        Vulkan::DestroyImageView(skybox_image_view);
        Vulkan::DestroyImage(skybox_image);
        Vulkan::DestroyPipeline(pipeline);
        Vulkan::DestroyPipelineLayout(pipeline_layout);
        Vulkan::DestroyDescriptorSetLayout(set_layout);
        Vulkan::DestroyDescriptorPool(descriptor_pool);
        Vulkan::DestroyImageView(render_image_view);
        Vulkan::DestroyShaderModule(shader_module);
    }
}  // namespace Ignis