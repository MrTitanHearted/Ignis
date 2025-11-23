#include <Ignis/Vulkan/Command.hpp>

namespace Ignis::Vulkan {
    namespace CommandPool {
        vk::CommandPool Create(
            const vk::CommandPoolCreateFlags flags,
            const std::uint32_t              queue_family_index,
            const vk::Device                 device) {
            vk::CommandPoolCreateInfo vk_command_pool_create_info{};
            vk_command_pool_create_info
                .setFlags(flags)
                .setQueueFamilyIndex(queue_family_index);
            auto [result, command_pool] = device.createCommandPool(vk_command_pool_create_info);
            DIGNIS_VK_CHECK(result);
            return command_pool;
        }

        vk::CommandPool CreateTransient(
            const std::uint32_t queue_family_index,
            const vk::Device    device) {
            return Create(vk::CommandPoolCreateFlagBits::eTransient, queue_family_index, device);
        }

        vk::CommandPool CreateResetCommandBuffer(
            const std::uint32_t queue_family_index,
            const vk::Device    device) {
            return Create(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family_index, device);
        }

    }  // namespace CommandPool

    namespace CommandBuffer {
        std::vector<vk::CommandBuffer> AllocatePrimary(
            const uint32_t        count,
            const vk::CommandPool command_pool,
            const vk::Device      device) {
            vk::CommandBufferAllocateInfo vk_command_buffer_allocate_info{};
            vk_command_buffer_allocate_info
                .setCommandBufferCount(count)
                .setCommandPool(command_pool)
                .setLevel(vk::CommandBufferLevel::ePrimary);
            auto [result, command_buffers] = device.allocateCommandBuffers(vk_command_buffer_allocate_info);
            DIGNIS_VK_CHECK(result);
            DIGNIS_ASSERT(command_buffers.size() == count, "Requested amount of command buffers differ from returned number of command buffers");
            return command_buffers;
        }

        vk::CommandBuffer AllocatePrimary(
            const vk::CommandPool command_pool,
            const vk::Device      device) {
            const auto command_buffers = AllocatePrimary(1, command_pool, device);
            return command_buffers[0];
        }

        void Begin(const vk::CommandBufferUsageFlags flags, const vk::CommandBuffer buffer) {
            DIGNIS_VK_CHECK(buffer.begin(vk::CommandBufferBeginInfo{flags}));
        }

        void BeginOneTimeSubmit(const vk::CommandBuffer buffer) {
            DIGNIS_VK_CHECK(buffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit}));
        }

        void End(const vk::CommandBuffer buffer) {
            DIGNIS_VK_CHECK(buffer.end());
        }

        vk::CommandBufferSubmitInfo GetSubmitInfo(const vk::CommandBuffer buffer) {
            return vk::CommandBufferSubmitInfo{}
                .setCommandBuffer(buffer);
        }

        void CopyImageToImage(
            const vk::Image         src_image,
            const vk::Image         dst_image,
            const vk::Extent3D     &src_extent,
            const vk::Extent3D     &dst_extent,
            const vk::CommandBuffer command_buffer) {
            vk::ImageBlit2 blit_region{};
            blit_region
                .setSrcOffsets({vk::Offset3D{0, 0, 0},
                                vk::Offset3D{
                                    static_cast<int32_t>(src_extent.width),
                                    static_cast<int32_t>(src_extent.height),
                                    static_cast<int32_t>(src_extent.depth),
                                }})
                .setDstOffsets({vk::Offset3D{0, 0, 0},
                                vk::Offset3D{
                                    static_cast<int32_t>(dst_extent.width),
                                    static_cast<int32_t>(dst_extent.height),
                                    static_cast<int32_t>(dst_extent.depth),
                                }})
                .setSrcSubresource(
                    vk::ImageSubresourceLayers{}
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1)
                        .setMipLevel(0))
                .setDstSubresource(vk::ImageSubresourceLayers{}
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
                .setFilter(vk::Filter::eLinear)
                .setRegions({blit_region});

            command_buffer.blitImage2(blit_info);
        }
    }  // namespace CommandBuffer
}  // namespace Ignis::Vulkan