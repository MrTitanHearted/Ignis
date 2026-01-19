#include <Ignis/Render.hpp>

namespace Ignis {
    void Render::initializeDefaultMaps() {
        const Vulkan::Image black_image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {1, 1});
        const Vulkan::Image white_image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {1, 1});
        const Vulkan::Image normal_image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {1, 1});
        const Vulkan::Image metallic_roughness_image = Vulkan::AllocateImage2D(
            {}, vma::MemoryUsage::eGpuOnly, {},
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {1, 1});

        const vk::ImageView black_image_view  = Vulkan::CreateImageColorView2D(black_image.Handle, black_image.Format);
        const vk::ImageView white_image_view  = Vulkan::CreateImageColorView2D(white_image.Handle, white_image.Format);
        const vk::ImageView normal_image_view = Vulkan::CreateImageColorView2D(normal_image.Handle, normal_image.Format);
        const vk::ImageView metallic_roughness_image_view =
            Vulkan::CreateImageColorView2D(metallic_roughness_image.Handle, metallic_roughness_image.Format);

        m_BlackTexture             = addTexture(black_image, black_image_view);
        m_WhiteTexture             = addTexture(white_image, white_image_view);
        m_NormalTexture            = addTexture(normal_image, normal_image_view);
        m_MetallicRoughnessTexture = addTexture(metallic_roughness_image, metallic_roughness_image_view);

        m_LoadedTextureRCs.emplace(m_BlackTexture, 1);
        m_LoadedTextureRCs.emplace(m_WhiteTexture, 1);
        m_LoadedTextureRCs.emplace(m_NormalTexture, 1);
        m_LoadedTextureRCs.emplace(m_MetallicRoughnessTexture, 1);

        m_LoadedTextures.emplace("[Ignis::Render::Black Texture]", m_BlackTexture);
        m_LoadedTextures.emplace("[Ignis::Render::White Texture]", m_WhiteTexture);
        m_LoadedTextures.emplace("[Ignis::Render::Normal Texture]", m_NormalTexture);
        m_LoadedTextures.emplace("[Ignis::Render::MetallicRoughness Texture]", m_MetallicRoughnessTexture);

        m_LoadedTexturePaths.emplace(m_BlackTexture, "[Ignis::Render::Black Texture]");
        m_LoadedTexturePaths.emplace(m_WhiteTexture, "[Ignis::Render::White Texture]");
        m_LoadedTexturePaths.emplace(m_NormalTexture, "[Ignis::Render::Normal Texture]");
        m_LoadedTexturePaths.emplace(m_MetallicRoughnessTexture, "[Ignis::Render::MetallicRoughness Texture]");

        const glm::uint32 colors[4]{
            glm::packUnorm4x8({0.0f, 0.0f, 0.0f, 1.0f}),
            glm::packUnorm4x8({1.0f, 1.0f, 1.0f, 1.0f}),
            glm::packUnorm4x8({0.5f, 0.5f, 1.0f, 1.0f}),
            glm::packUnorm4x8({0.0f, 1.0f, 1.0f, 1.0f}),
        };

        const Vulkan::Buffer staging = Vulkan::AllocateBuffer(
            vma::AllocationCreateFlagBits::eMapped,
            vma::MemoryUsage::eCpuOnly, {},
            sizeof(glm::uint32) * 4,
            vk::BufferUsageFlagBits::eTransferSrc);
        Vulkan::CopyMemoryToAllocation(colors, staging.Allocation, 0, staging.Size);

        Vulkan::ImmediateSubmit([&](const vk::CommandBuffer command_buffer) {
            Vulkan::BarrierMerger merger{};

            merger.putImageBarrier(
                black_image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                white_image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                normal_image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                metallic_roughness_image.Handle,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.flushBarriers(command_buffer);

            uint64_t offset = 0;
            Vulkan::CopyBufferToImage(
                staging.Handle,
                black_image.Handle,
                offset,
                {0, 0, 0},
                {0, 0},
                black_image.Extent,
                command_buffer);
            offset += sizeof(glm::uint32);
            Vulkan::CopyBufferToImage(
                staging.Handle,
                white_image.Handle,
                offset,
                {0, 0, 0},
                {0, 0},
                black_image.Extent,
                command_buffer);
            offset += sizeof(glm::uint32);
            Vulkan::CopyBufferToImage(
                staging.Handle,
                normal_image.Handle,
                offset,
                {0, 0, 0},
                {0, 0},
                black_image.Extent,
                command_buffer);
            offset += sizeof(glm::uint32);
            Vulkan::CopyBufferToImage(
                staging.Handle,
                metallic_roughness_image.Handle,
                offset,
                {0, 0, 0},
                {0, 0},
                black_image.Extent,
                command_buffer);
            offset += sizeof(glm::uint32);

            merger.putImageBarrier(
                black_image.Handle,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                white_image.Handle,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                normal_image.Handle,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone);
            merger.putImageBarrier(
                metallic_roughness_image.Handle,
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
}  // namespace Ignis