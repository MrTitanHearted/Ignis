#include <Ignis/Vulkan/Image.hpp>

namespace Ignis::Vulkan {
    namespace Image {
        vk::ImageAspectFlags GetAspectMask(const vk::ImageLayout layout) {
            switch (layout) {
                case vk::ImageLayout::eUndefined:
                    return vk::ImageAspectFlagBits::eNone;
                case vk::ImageLayout::eGeneral:
                    return vk::ImageAspectFlagBits::eColor |
                           vk::ImageAspectFlagBits::eDepth |
                           vk::ImageAspectFlagBits::eStencil;
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
    }  // namespace Image

    namespace ImageView {}
}  // namespace Ignis::Vulkan