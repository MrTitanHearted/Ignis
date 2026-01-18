#include <Ignis/Render.hpp>

namespace Ignis {
    vk::ShaderModule g_SkyboxShader = nullptr;

    void Render::initializeSkybox(const std::array<std::filesystem::path, 6> &skybox_face_paths) {
        m_SkyboxDescriptorLayout =
            Vulkan::DescriptorSetLayoutBuilder()
                .addCombinedImageSampler(0, vk::ShaderStageFlagBits::eFragment)
                .build();

        m_SkyboxPipelineLayout = Vulkan::CreatePipelineLayout(
            vk::PushConstantRange{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(CameraPC)},
            {m_SkyboxDescriptorLayout});

        m_SkyboxDescriptorSet = Vulkan::AllocateDescriptorSet(m_SkyboxDescriptorLayout, m_DescriptorPool);

        constexpr auto texture_asset_type = TextureAsset::Type::eRGBA32f;
        constexpr auto image_format       = vk::Format::eR32G32B32A32Sfloat;

        size_t face_size = 0;

        {
            const TextureAsset texture_asset = TextureAsset::LoadFromPath(skybox_face_paths[0], texture_asset_type).value();

            face_size = texture_asset.getData().size();

            m_SkyboxImage = Vulkan::AllocateImageCube(
                {}, vma::MemoryUsage::eGpuOnly, {},
                image_format,
                vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferDst,
                vk::Extent2D{texture_asset.getWidth(), texture_asset.getHeight()});

            m_SkyboxImageView = Vulkan::CreateImageColorViewCube(m_SkyboxImage.Handle, m_SkyboxImage.Format);
        }

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

        {
            const Vulkan::Buffer staging = Vulkan::AllocateBuffer(
                vma::AllocationCreateFlagBits::eMapped,
                vma::MemoryUsage::eCpuOnly, {},
                face_size * 6 +
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
                for (const std::filesystem::path &path : skybox_face_paths) {
                    TextureAsset texture_asset = TextureAsset::LoadFromPath(path, texture_asset_type).value();

                    Vulkan::CopyMemoryToAllocation(
                        texture_asset.getData().data(),
                        staging.Allocation,
                        offset,
                        texture_asset.getData().size());
                    offset += texture_asset.getData().size();
                }

                Vulkan::CopyMemoryToAllocation(vertices, staging.Allocation, offset, m_SkyboxVertexBuffer.Size);
                offset += m_SkyboxVertexBuffer.Size;
                Vulkan::CopyMemoryToAllocation(indices, staging.Allocation, offset, m_SkyboxIndexBuffer.Size);
                offset += m_SkyboxIndexBuffer.Size;
            }

            Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
                Vulkan::BarrierMerger merger{};

                merger.putImageBarrier(
                    m_SkyboxImage.Handle,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone);
                merger.flushBarriers(command_buffer);

                uint64_t src_offset = 0;
                Vulkan::CopyBufferToImageCube(
                    staging.Handle,
                    m_SkyboxImage.Handle,
                    src_offset,
                    face_size,
                    vk::Extent2D{m_SkyboxImage.Extent.width, m_SkyboxImage.Extent.height},
                    command_buffer);
                src_offset += face_size * 6;
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
                    m_SkyboxImage.Handle,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone);
                merger.flushBarriers(command_buffer);
            });

            Vulkan::DestroyBuffer(staging);
        }

        generateBRDFLUTMap();
        generatePrefilterMap();
        generateIrradianceMap();

        Vulkan::DescriptorSetWriter()
            .writeCombinedImageSampler(0, m_SkyboxImageView, vk::ImageLayout::eShaderReadOnlyOptimal, m_Sampler)
            .update(m_SkyboxDescriptorSet);

        m_FrameGraphSkyboxImage =
            m_pFrameGraph->importImage(
                m_SkyboxImage.Handle,
                m_SkyboxImageView,
                m_SkyboxImage.Format,
                m_SkyboxImage.Usage,
                m_SkyboxImage.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);

        m_FrameGraphBRDFLUTImage =
            m_pFrameGraph->importImage(
                m_BRDFLUTImage.Handle,
                m_BRDFLUTImageView,
                m_BRDFLUTImage.Format,
                m_BRDFLUTImage.Usage,
                m_BRDFLUTImage.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);

        m_FrameGraphPrefilterImage =
            m_pFrameGraph->importImage(
                m_PrefilterImage.Handle,
                m_PrefilterImageView,
                m_PrefilterImage.Format,
                m_PrefilterImage.Usage,
                m_PrefilterImage.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);

        m_FrameGraphIrradianceImage =
            m_pFrameGraph->importImage(
                m_IrradianceImage.Handle,
                m_IrradianceImageView,
                m_IrradianceImage.Format,
                m_IrradianceImage.Usage,
                m_IrradianceImage.Extent,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);

        m_FrameGraphSkyboxVertexBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_SkyboxVertexBuffer.Handle, m_SkyboxVertexBuffer.Usage, 0, m_SkyboxVertexBuffer.Size),
            0,
            m_SkyboxVertexBuffer.Size,
            vk::PipelineStageFlagBits2::eVertexInput,
        };

        m_FrameGraphSkyboxIndexBuffer = FrameGraph::BufferInfo{
            m_pFrameGraph->importBuffer(m_SkyboxIndexBuffer.Handle, m_SkyboxIndexBuffer.Usage, 0, m_SkyboxIndexBuffer.Size),
            0,
            m_SkyboxIndexBuffer.Size,
            vk::PipelineStageFlagBits2::eIndexInput,
        };

        const FileAsset skybox_shader_file = FileAsset::LoadBinaryFromPath("Assets/Shaders/Ignis/Skybox.spv").value();

        std::vector<uint32_t> skybox_shader_code{};
        skybox_shader_code.resize(skybox_shader_file.getSize() * sizeof(char) / sizeof(uint32_t));

        std::memcpy(skybox_shader_code.data(), skybox_shader_file.getContent().data(), skybox_shader_file.getSize());

        g_SkyboxShader = Vulkan::CreateShaderModuleFromSPV(skybox_shader_code);
    }

    void Render::releaseSkybox() {
        m_pFrameGraph->removeImage(m_FrameGraphIrradianceImage);
        m_pFrameGraph->removeImage(m_FrameGraphPrefilterImage);
        m_pFrameGraph->removeImage(m_FrameGraphBRDFLUTImage);
        m_pFrameGraph->removeImage(m_FrameGraphSkyboxImage);
        m_pFrameGraph->removeBuffer(m_FrameGraphSkyboxIndexBuffer.Buffer);
        m_pFrameGraph->removeBuffer(m_FrameGraphSkyboxVertexBuffer.Buffer);

        Vulkan::DestroyPipeline(m_SkyboxPipeline);
        Vulkan::DestroyShaderModule(g_SkyboxShader);
        Vulkan::DestroyPipelineLayout(m_SkyboxPipelineLayout);
        Vulkan::DestroyDescriptorSetLayout(m_SkyboxDescriptorLayout);

        Vulkan::DestroyBuffer(m_SkyboxVertexBuffer);
        Vulkan::DestroyBuffer(m_SkyboxIndexBuffer);

        Vulkan::DestroyImageView(m_IrradianceImageView);
        Vulkan::DestroyImage(m_IrradianceImage);

        Vulkan::DestroyImageView(m_PrefilterImageView);
        Vulkan::DestroyImage(m_PrefilterImage);

        Vulkan::DestroyImageView(m_BRDFLUTImageView);
        Vulkan::DestroyImage(m_BRDFLUTImage);

        Vulkan::DestroyImageView(m_SkyboxImageView);
        Vulkan::DestroyImage(m_SkyboxImage);

        m_SkyboxDescriptorSet = nullptr;
    }

    void Render::setSkyboxViewport(
        const FrameGraph::ImageID color_image,
        const FrameGraph::ImageID depth_image) {
        if (nullptr == m_SkyboxPipeline)
            m_SkyboxPipeline =
                Vulkan::GraphicsPipelineBuilder()
                    .setVertexShader("vs_main", g_SkyboxShader)
                    .setFragmentShader("fs_main", g_SkyboxShader)
                    .setVertexLayouts({Vulkan::VertexLayout{
                        vk::VertexInputBindingDescription{0, sizeof(glm::vec3), vk::VertexInputRate::eVertex},
                        {
                            vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, 0},
                        },
                    }})
                    .setInputTopology(vk::PrimitiveTopology::eTriangleList)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setColorAttachmentFormats({m_pFrameGraph->getImageFormat(color_image)})
                    .setDepthAttachmentFormat(m_pFrameGraph->getImageFormat(depth_image))
                    .setDepthTest(vk::False, vk::CompareOp::eLessOrEqual)
                    .setNoStencilTest()
                    .setNoMultisampling()
                    .setNoBlending()
                    .build(m_SkyboxPipelineLayout);
    }

    void Render::readSkyboxImage(FrameGraph::RenderPass &render_pass) const {
        render_pass.readImages({
            m_FrameGraphSkyboxImage,
            m_FrameGraphBRDFLUTImage,
            m_FrameGraphPrefilterImage,
            m_FrameGraphIrradianceImage,
        });
    }

    void Render::readSkyboxBuffers(FrameGraph::RenderPass &render_pass) const {
        render_pass.readBuffers({m_FrameGraphSkyboxVertexBuffer, m_FrameGraphSkyboxIndexBuffer});
    }

    void Render::onSkyboxDraw(const vk::CommandBuffer command_buffer) const {
        DIGNIS_ASSERT(nullptr != m_SkyboxPipeline);

        const CameraPC camera_pc{
            m_Camera.Projection * glm::mat4x4(glm::mat3x3(m_Camera.View)),
            m_Camera.Position,
        };

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_SkyboxPipeline);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_SkyboxPipelineLayout, 0,
            {m_SkyboxDescriptorSet}, {});

        command_buffer.pushConstants(
            m_SkyboxPipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(CameraPC),
            &camera_pc);

        command_buffer.bindVertexBuffers(0, {m_SkyboxVertexBuffer.Handle}, {0});
        command_buffer.bindIndexBuffer(m_SkyboxIndexBuffer.Handle, 0, vk::IndexType::eUint16);
        command_buffer.drawIndexed(36, 1, 0, 0, 0);
    }
}  // namespace Ignis