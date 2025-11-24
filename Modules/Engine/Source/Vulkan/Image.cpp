#include <Ignis/Vulkan/Image.hpp>

namespace Ignis::Vulkan {
    namespace Image {
        vk::ImageAspectFlags GetAspectMask(const vk::ImageLayout layout) {
            switch (layout) {
                case vk::ImageLayout::eUndefined:
                    return vk::ImageAspectFlagBits::eNone;
                case vk::ImageLayout::eColorAttachmentOptimal:
                    return vk::ImageAspectFlagBits::eColor;
                case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
                case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
                case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
                    return vk::ImageAspectFlagBits::eDepth |
                           vk::ImageAspectFlagBits::eStencil;
                case vk::ImageLayout::eDepthAttachmentOptimal:
                case vk::ImageLayout::eDepthReadOnlyOptimal:
                    return vk::ImageAspectFlagBits::eDepth;
                case vk::ImageLayout::eStencilAttachmentOptimal:
                case vk::ImageLayout::eStencilReadOnlyOptimal:
                    return vk::ImageAspectFlagBits::eStencil;
                default:
                    return vk::ImageAspectFlagBits::eColor;
            }
        }

        void TransitionLayout(
            const vk::Image         image,
            const vk::ImageLayout   old_layout,
            const vk::ImageLayout   new_layout,
            const vk::CommandBuffer command_buffer) {
            vk::ImageMemoryBarrier2 vk_image_memory_barrier2{};
            vk_image_memory_barrier2
                .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
                .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead)
                .setOldLayout(old_layout)
                .setNewLayout(new_layout)
                .setImage(image)
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(GetAspectMask(new_layout))
                        .setBaseMipLevel(0)
                        .setLevelCount(vk::RemainingMipLevels)
                        .setBaseArrayLayer(0)
                        .setLayerCount(vk::RemainingArrayLayers));
            command_buffer.pipelineBarrier2(
                vk::DependencyInfo{}
                    .setImageMemoryBarriers(vk_image_memory_barrier2));
        }

        void TransitionLayout(
            const vk::Image         image,
            const vk::ImageLayout   new_layout,
            const vk::CommandBuffer command_buffer) {
            TransitionLayout(image, vk::ImageLayout::eUndefined, new_layout, command_buffer);
        }

        void Destroy(const Allocation &allocation, const vma::Allocator allocator) {
            allocator.destroyImage(allocation.Image, allocation.VmaAllocation);
        }

        Allocation Allocate3D(
            const vma::AllocationCreateFlags allocation_flags,
            const vma::MemoryUsage           memory_usage,
            const vk::Format                 format,
            const vk::ImageUsageFlags        usage_flags,
            const vk::Extent3D              &extent,
            const vma::Allocator             allocator) {
            vk::ImageCreateInfo vk_image_create_info{};
            vk_image_create_info
                .setImageType(vk::ImageType::e3D)
                .setFormat(format)
                .setExtent(extent)
                .setArrayLayers(1)
                .setMipLevels(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setTiling(vk::ImageTiling::eOptimal)
                .setUsage(usage_flags);
            vma::AllocationCreateInfo vma_allocation_create_info{};
            vma_allocation_create_info
                .setFlags(allocation_flags)
                .setUsage(memory_usage);

            const auto [result, image_allocation] =
                allocator.createImage(vk_image_create_info, vma_allocation_create_info);
            DIGNIS_VK_CHECK(result);
            const auto [image, vma_allocation] = image_allocation;

            Allocation allocation{};
            allocation.Image         = image;
            allocation.Extent        = extent;
            allocation.Format        = format;
            allocation.VmaAllocation = vma_allocation;
            allocation.UsageFlags    = usage_flags;
            return allocation;
        }

        Allocation Allocate3D(
            const vma::MemoryUsage    memory_usage,
            const vk::Format          format,
            const vk::ImageUsageFlags usage_flags,
            const vk::Extent3D       &extent,
            const vma::Allocator      allocator) {
            return Allocate3D({}, memory_usage, format, usage_flags, extent, allocator);
        }

        Allocation Allocate2D(
            const vma::AllocationCreateFlags allocation_flags,
            const vma::MemoryUsage           memory_usage,
            const vk::Format                 format,
            const vk::ImageUsageFlags        usage_flags,
            const vk::Extent2D              &extent,
            const vma::Allocator             allocator) {
            vk::ImageCreateInfo vk_image_create_info{};
            vk_image_create_info
                .setImageType(vk::ImageType::e2D)
                .setFormat(format)
                .setExtent(vk::Extent3D{extent, 1})
                .setArrayLayers(1)
                .setMipLevels(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setTiling(vk::ImageTiling::eOptimal)
                .setUsage(usage_flags);
            vma::AllocationCreateInfo vma_allocation_create_info{};
            vma_allocation_create_info
                .setFlags(allocation_flags)
                .setUsage(memory_usage);

            const auto [result, image_allocation] =
                allocator.createImage(vk_image_create_info, vma_allocation_create_info);
            DIGNIS_VK_CHECK(result);
            const auto [image, vma_allocation] = image_allocation;

            Allocation allocation{};
            allocation.Image         = image;
            allocation.Extent        = vk::Extent3D{extent, 1};
            allocation.Format        = format;
            allocation.VmaAllocation = vma_allocation;
            allocation.UsageFlags    = usage_flags;
            return allocation;
        }

        Allocation Allocate2D(
            const vma::MemoryUsage    memory_usage,
            const vk::Format          format,
            const vk::ImageUsageFlags usage_flags,
            const vk::Extent2D       &extent,
            const vma::Allocator      allocator) {
            return Allocate2D({}, memory_usage, format, usage_flags, extent, allocator);
        }

    }  // namespace Image

    namespace ImageView {
        vk::ImageView CreateColor3D(
            const vk::Format format,
            const vk::Image  image,
            const vk::Device device) {
            vk::ImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info
                .setViewType(vk::ImageViewType::e3D)
                .setFormat(format)
                .setImage(image)
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(0)
                        .setBaseMipLevel(0)
                        .setLayerCount(1)
                        .setLevelCount(1));
            auto [result, view] = device.createImageView(vk_image_view_create_info);
            DIGNIS_VK_CHECK(result);
            return view;
        }

        vk::ImageView CreateDepth3D(
            const vk::Format format,
            const vk::Image  image,
            const vk::Device device) {
            vk::ImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info
                .setViewType(vk::ImageViewType::e3D)
                .setFormat(format)
                .setImage(image)
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                        .setBaseArrayLayer(0)
                        .setBaseMipLevel(0)
                        .setLayerCount(1)
                        .setLevelCount(1));
            auto [result, view] = device.createImageView(vk_image_view_create_info);
            DIGNIS_VK_CHECK(result);
            return view;
        }

        vk::ImageView CreateColor2D(
            const vk::Format format,
            const vk::Image  image,
            const vk::Device device) {
            vk::ImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(format)
                .setImage(image)
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(0)
                        .setBaseMipLevel(0)
                        .setLayerCount(1)
                        .setLevelCount(1));
            auto [result, view] = device.createImageView(vk_image_view_create_info);
            DIGNIS_VK_CHECK(result);
            return view;
        }

        vk::ImageView CreateDepth2D(
            const vk::Format format,
            const vk::Image  image,
            const vk::Device device) {
            vk::ImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(format)
                .setImage(image)
                .setSubresourceRange(
                    vk::ImageSubresourceRange{}
                        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                        .setBaseArrayLayer(0)
                        .setBaseMipLevel(0)
                        .setLayerCount(1)
                        .setLevelCount(1));
            auto [result, view] = device.createImageView(vk_image_view_create_info);
            DIGNIS_VK_CHECK(result);
            return view;
        }
    }  // namespace ImageView
}  // namespace Ignis::Vulkan