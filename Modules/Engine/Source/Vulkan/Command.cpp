#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::BarrierMerger::clear() {
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
    }

    void Vulkan::BarrierMerger::putImageBarrier(
        const vk::Image               image,
        const vk::ImageLayout         old_layout,
        const vk::ImageLayout         new_layout,
        const uint32_t                base_mip_level,
        const uint32_t                mip_level_count,
        const uint32_t                base_array_layer,
        const uint32_t                array_layer_count,
        const vk::PipelineStageFlags2 src_stage,
        const vk::AccessFlags2        src_access,
        const vk::PipelineStageFlags2 dst_stage,
        const vk::AccessFlags2        dst_access) {
        vk::ImageMemoryBarrier2 image_barrier{};
        image_barrier
            .setSrcStageMask(src_stage)
            .setSrcAccessMask(src_access)
            .setDstStageMask(dst_stage)
            .setDstAccessMask(dst_access)
            .setOldLayout(old_layout)
            .setNewLayout(new_layout)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(GetImageAspectMask(new_layout))
                    .setBaseMipLevel(base_mip_level)
                    .setLevelCount(mip_level_count)
                    .setBaseArrayLayer(base_array_layer)
                    .setLayerCount(array_layer_count));
        m_ImageBarriers.push_back(image_barrier);
    }

    void Vulkan::BarrierMerger::putImageBarrier(
        const vk::Image               image,
        const vk::ImageLayout         old_layout,
        const vk::ImageLayout         new_layout,
        const uint32_t                base_mip_level,
        const uint32_t                base_array_layer,
        const vk::PipelineStageFlags2 src_stage,
        const vk::AccessFlags2        src_access,
        const vk::PipelineStageFlags2 dst_stage,
        const vk::AccessFlags2        dst_access) {
        vk::ImageMemoryBarrier2 image_barrier{};
        image_barrier
            .setSrcStageMask(src_stage)
            .setSrcAccessMask(src_access)
            .setDstStageMask(dst_stage)
            .setDstAccessMask(dst_access)
            .setOldLayout(old_layout)
            .setNewLayout(new_layout)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(GetImageAspectMask(new_layout))
                    .setBaseMipLevel(base_mip_level)
                    .setLevelCount(vk::RemainingMipLevels)
                    .setBaseArrayLayer(base_array_layer)
                    .setLayerCount(vk::RemainingArrayLayers));
        m_ImageBarriers.push_back(image_barrier);
    }

    void Vulkan::BarrierMerger::putImageBarrier(
        const vk::Image               image,
        const vk::ImageLayout         old_layout,
        const vk::ImageLayout         new_layout,
        const vk::PipelineStageFlags2 src_stage,
        const vk::AccessFlags2        src_access,
        const vk::PipelineStageFlags2 dst_stage,
        const vk::AccessFlags2        dst_access) {
        vk::ImageMemoryBarrier2 image_barrier{};
        image_barrier
            .setSrcStageMask(src_stage)
            .setSrcAccessMask(src_access)
            .setDstStageMask(dst_stage)
            .setDstAccessMask(dst_access)
            .setOldLayout(old_layout)
            .setNewLayout(new_layout)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(GetImageAspectMask(new_layout))
                    .setBaseMipLevel(0)
                    .setLevelCount(vk::RemainingMipLevels)
                    .setBaseArrayLayer(0)
                    .setLayerCount(vk::RemainingArrayLayers));
        m_ImageBarriers.push_back(image_barrier);
    }

    void Vulkan::BarrierMerger::putBufferBarrier(
        const vk::Buffer              buffer,
        const uint64_t                offset,
        const uint64_t                size,
        const vk::PipelineStageFlags2 src_stage,
        const vk::AccessFlags2        src_access,
        const vk::PipelineStageFlags2 dst_stage,
        const vk::AccessFlags2        dst_access) {
        vk::BufferMemoryBarrier2 buffer_barrier{};
        buffer_barrier
            .setSrcStageMask(src_stage)
            .setSrcAccessMask(src_access)
            .setDstStageMask(dst_stage)
            .setDstAccessMask(dst_access)
            .setBuffer(buffer)
            .setOffset(offset)
            .setSize(size);
        m_BufferBarriers.push_back(buffer_barrier);
    }

    void Vulkan::BarrierMerger::flushBarriers(const vk::CommandBuffer command_buffer) {
        vk::DependencyInfo dependency_info{};
        dependency_info
            .setImageMemoryBarriers(m_ImageBarriers)
            .setBufferMemoryBarriers(m_BufferBarriers);
        command_buffer.pipelineBarrier2(dependency_info);
        m_ImageBarriers.clear();
        m_BufferBarriers.clear();
    }

    void Vulkan::DestroyCommandPool(const vk::CommandPool command_pool) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        s_pInstance->m_Device.destroyCommandPool(command_pool);
    }

    vk::CommandPool Vulkan::CreateCommandPool(const vk::CommandPoolCreateFlags flags) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        vk::CommandPoolCreateInfo create_info{};
        create_info
            .setFlags(flags)
            .setQueueFamilyIndex(s_pInstance->m_QueueFamilyIndex);
        auto [result, command_pool] = s_pInstance->m_Device.createCommandPool(create_info);
        DIGNIS_VK_CHECK(result);
        return command_pool;
    }

    std::vector<vk::CommandBuffer> Vulkan::AllocatePrimaryCommandBuffers(
        const uint32_t        count,
        const vk::CommandPool command_pool) {
        vk::CommandBufferAllocateInfo allocate_info{};
        allocate_info
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(count)
            .setCommandPool(command_pool);
        auto [result, command_buffers] = s_pInstance->m_Device.allocateCommandBuffers(allocate_info);
        DIGNIS_VK_CHECK(result);
        DIGNIS_ASSERT(
            command_buffers.size() == count,
            "Ignis::Vulkan::AllocatePrimaryCommandBuffers: Requested number of command buffers differ from returned number: {} requested, {} returned.",
            count,
            command_buffers.size());
        return command_buffers;
    }

    vk::CommandBuffer Vulkan::AllocatePrimaryCommandBuffer(const vk::CommandPool command_pool) {
        const auto command_buffers = AllocatePrimaryCommandBuffers(1, command_pool);
        return command_buffers[0];
    }

    void Vulkan::ResetCommandBuffer(const vk::CommandBuffer command_buffer) {
        DIGNIS_VK_CHECK(command_buffer.reset());
    }

    void Vulkan::BeginDebugUtilsLabel(
        const vk::CommandBuffer     command_buffer,
        const std::string_view      label,
        const std::array<float, 4> &color) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        command_buffer.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{label.data(), color}, s_pInstance->m_DynamicLoader);
    }

    void Vulkan::EndDebugUtilsLabel(const vk::CommandBuffer command_buffer) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        command_buffer.endDebugUtilsLabelEXT(s_pInstance->m_DynamicLoader);
    }

    void Vulkan::BeginCommandBuffer(
        const vk::CommandBufferUsageFlags flags,
        const vk::CommandBuffer           command_buffer) {
        const vk::CommandBufferBeginInfo begin_info{flags};
        DIGNIS_VK_CHECK(command_buffer.begin(begin_info));
    }

    void Vulkan::EndCommandBuffer(const vk::CommandBuffer command_buffer) {
        DIGNIS_VK_CHECK(command_buffer.end());
    }

    void Vulkan::BlitImageToImage(
        const vk::Image src_image,
        const vk::Image dst_image,

        const uint32_t src_base_mip_level,
        const uint32_t dst_base_mip_level,
        const uint32_t src_base_array_layer,
        const uint32_t dst_base_array_layer,
        const uint32_t src_array_layer_count,
        const uint32_t dst_array_layer_count,

        const vk::Offset3D &src_offset,
        const vk::Offset3D &dst_offset,
        const vk::Extent3D &src_extent,
        const vk::Extent3D &dst_extent,

        const vk::CommandBuffer command_buffer) {
        vk::ImageBlit2 blit_region{};
        blit_region
            .setSrcOffsets({
                src_offset,
                vk::Offset3D{
                    static_cast<int32_t>(src_extent.width),
                    static_cast<int32_t>(src_extent.height),
                    static_cast<int32_t>(src_extent.depth),
                },
            })
            .setDstOffsets({
                dst_offset,
                vk::Offset3D{
                    static_cast<int32_t>(dst_extent.width),
                    static_cast<int32_t>(dst_extent.height),
                    static_cast<int32_t>(dst_extent.depth),
                },
            })
            .setSrcSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(src_base_array_layer)
                    .setLayerCount(src_array_layer_count)
                    .setMipLevel(src_base_mip_level))
            .setDstSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(dst_base_array_layer)
                    .setLayerCount(dst_array_layer_count)
                    .setMipLevel(dst_base_mip_level));

        vk::BlitImageInfo2 blit_info{};
        blit_info
            .setSrcImage(src_image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(dst_image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eNearest)
            .setRegions({blit_region});

        command_buffer.blitImage2(blit_info);
    }

    void Vulkan::BlitImageToImage(
        const vk::Image         src_image,
        const vk::Image         dst_image,
        const vk::Offset3D     &src_offset,
        const vk::Offset3D     &dst_offset,
        const vk::Extent3D     &src_extent,
        const vk::Extent3D     &dst_extent,
        const vk::CommandBuffer command_buffer) {
        vk::ImageBlit2 blit_region{};
        blit_region
            .setSrcOffsets({
                src_offset,
                vk::Offset3D{
                    static_cast<int32_t>(src_extent.width),
                    static_cast<int32_t>(src_extent.height),
                    static_cast<int32_t>(src_extent.depth),
                },
            })
            .setDstOffsets({
                dst_offset,
                vk::Offset3D{
                    static_cast<int32_t>(dst_extent.width),
                    static_cast<int32_t>(dst_extent.height),
                    static_cast<int32_t>(dst_extent.depth),
                },
            })
            .setSrcSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0))
            .setDstSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0));

        vk::BlitImageInfo2 blit_info{};
        blit_info
            .setSrcImage(src_image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImage(dst_image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setFilter(vk::Filter::eNearest)
            .setRegions({blit_region});

        command_buffer.blitImage2(blit_info);
    }

    void Vulkan::GenerateImageCubeMipLevels(
        const vk::Image         image,
        const vk::ImageLayout   old_layout,
        const vk::ImageLayout   new_layout,
        const uint32_t          mip_level_count,
        const vk::Extent2D     &extent,
        const vk::CommandBuffer command_buffer) {
        BarrierMerger merger{};
        if (vk::ImageLayout::eTransferDstOptimal != old_layout) {
            merger.putImageBarrier(
                image,
                old_layout,
                vk::ImageLayout::eTransferDstOptimal,
                0, vk::RemainingMipLevels,
                0, 6,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eBlit,
                vk::AccessFlagBits2::eTransferWrite);
            merger.flushBarriers(command_buffer);
        }

        merger.putImageBarrier(
            image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            0, 1,
            0, 6,
            vk::PipelineStageFlagBits2::eBlit,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eBlit,
            vk::AccessFlagBits2::eTransferWrite);
        merger.flushBarriers(command_buffer);

        vk::Extent3D src_extent{extent, 1};
        vk::Extent3D dst_extent{extent, 1};

        for (uint32_t mip_level = 1; mip_level < mip_level_count; mip_level++) {
            dst_extent.width  = static_cast<uint32_t>(static_cast<glm::f32>(dst_extent.width) * 0.5f);
            dst_extent.height = static_cast<uint32_t>(static_cast<glm::f32>(dst_extent.height) * 0.5f);
            BlitImageToImage(
                image,
                image,
                mip_level - 1,
                mip_level,
                0, 0,
                6, 6,
                {0, 0, 0},
                {0, 0, 0},
                src_extent,
                dst_extent,
                command_buffer);

            merger.putImageBarrier(
                image,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                mip_level, 1,
                0, 6,
                vk::PipelineStageFlagBits2::eBlit,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead);
            merger.flushBarriers(command_buffer);

            src_extent.width  = dst_extent.width;
            src_extent.height = dst_extent.height;
        }

        merger.putImageBarrier(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            new_layout,
            0, vk::RemainingMipLevels,
            0, 6,
            vk::PipelineStageFlagBits2::eBlit,
            vk::AccessFlagBits2::eTransferRead,
            vk::PipelineStageFlagBits2::eBlit,
            vk::AccessFlagBits2::eTransferWrite);
        merger.flushBarriers(command_buffer);
    }

    void Vulkan::CopyImageToImage(
        const vk::Image         src_image,
        const vk::Image         dst_image,
        const vk::Offset3D     &src_offset,
        const vk::Offset3D     &dst_offset,
        const vk::Extent3D     &extent,
        const vk::CommandBuffer command_buffer) {
        vk::ImageCopy2 region{};
        region
            .setSrcOffset(src_offset)
            .setDstOffset(dst_offset)
            .setSrcSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0))
            .setDstSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0))
            .setExtent(extent);

        vk::CopyImageInfo2 copy_info{};
        copy_info
            .setSrcImage(src_image)
            .setDstImage(dst_image)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setRegions(region);

        command_buffer.copyImage2(copy_info);
    }

    void Vulkan::CopyBufferToBuffer(
        const vk::Buffer        src_buffer,
        const vk::Buffer        dst_buffer,
        const uint64_t          src_offset,
        const uint64_t          dst_offset,
        const uint64_t          size,
        const vk::CommandBuffer command_buffer) {
        vk::BufferCopy2 region{};
        region
            .setSrcOffset(src_offset)
            .setDstOffset(dst_offset)
            .setSize(size);
        vk::CopyBufferInfo2 copy_info{};
        copy_info
            .setSrcBuffer(src_buffer)
            .setDstBuffer(dst_buffer)
            .setRegions(region);
        command_buffer.copyBuffer2(copy_info);
    }

    void Vulkan::CopyBufferToImage(
        const vk::Buffer        src_buffer,
        const vk::Image         dst_image,
        const uint64_t          src_offset,
        const vk::Offset3D     &dst_offset,
        const vk::Extent2D     &src_extent,
        const vk::Extent3D     &dst_extent,
        const vk::CommandBuffer command_buffer) {
        vk::BufferImageCopy2 region{};
        region
            .setBufferOffset(src_offset)
            .setImageOffset(dst_offset)
            .setBufferRowLength(src_extent.width)
            .setBufferImageHeight(src_extent.height)
            .setImageExtent(dst_extent)
            .setImageSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0));

        vk::CopyBufferToImageInfo2 copy_info{};
        copy_info
            .setSrcBuffer(src_buffer)
            .setDstImage(dst_image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setRegions(region);

        command_buffer.copyBufferToImage2(copy_info);
    }

    void Vulkan::CopyImageToBuffer(
        const vk::Image         src_image,
        const vk::Buffer        dst_buffer,
        const vk::Offset3D     &src_offset,
        const uint64_t          dst_offset,
        const vk::Extent3D     &src_extent,
        const vk::Extent2D     &dst_extent,
        const vk::CommandBuffer command_buffer) {
        vk::BufferImageCopy2 region{};
        region
            .setImageOffset(src_offset)
            .setBufferOffset(dst_offset)
            .setImageExtent(src_extent)
            .setBufferRowLength(src_extent.width)
            .setBufferImageHeight(src_extent.height)
            .setImageSubresource(
                vk::ImageSubresourceLayers{}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
                    .setMipLevel(0));

        vk::CopyImageToBufferInfo2 copy_info{};
        copy_info
            .setSrcImage(src_image)
            .setDstBuffer(dst_buffer)
            .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setRegions(region);

        command_buffer.copyImageToBuffer2(copy_info);
    }

    void Vulkan::CopyBufferToImageCube(
        const vk::Buffer        src_buffer,
        const vk::Image         dst_image,
        const uint64_t          src_offset,
        const uint64_t          dst_face_size,
        const vk::Extent2D     &dst_extent,
        const vk::CommandBuffer command_buffer) {
        std::array<vk::BufferImageCopy2, 6> regions{};

        for (uint32_t i = 0; i < 6; i++) {
            vk::BufferImageCopy2 &region = regions[i];

            region
                .setBufferOffset(src_offset + dst_face_size * i)
                .setImageOffset(vk::Offset3D{0, 0, 0})
                .setBufferRowLength(0)
                .setBufferImageHeight(0)
                .setImageExtent(vk::Extent3D{dst_extent, 1})
                .setImageSubresource(
                    vk::ImageSubresourceLayers{}
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(i)
                        .setLayerCount(1)
                        .setMipLevel(0));
        }

        vk::CopyBufferToImageInfo2 copy_info{};
        copy_info
            .setSrcBuffer(src_buffer)
            .setDstImage(dst_image)
            .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
            .setRegions(regions);

        command_buffer.copyBufferToImage2(copy_info);
    }

    vk::CommandBufferSubmitInfo Vulkan::GetCommandBufferSubmitInfo(const vk::CommandBuffer command_buffer) {
        return vk::CommandBufferSubmitInfo{command_buffer};
    }
}  // namespace Ignis