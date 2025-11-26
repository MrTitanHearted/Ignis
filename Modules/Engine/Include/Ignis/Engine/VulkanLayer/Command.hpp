#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan {
    namespace CommandPool {
        vk::CommandPool Create(
            vk::CommandPoolCreateFlags flags,
            std::uint32_t              queue_family_index,
            vk::Device                 device);
        vk::CommandPool CreateTransient(
            std::uint32_t queue_family_index,
            vk::Device    device);
        vk::CommandPool CreateResetCommandBuffer(
            std::uint32_t queue_family_index,
            vk::Device    device);
    }  // namespace CommandPool

    namespace CommandBuffer {
        std::vector<vk::CommandBuffer> AllocatePrimary(
            uint32_t        count,
            vk::CommandPool command_pool,
            vk::Device      device);

        vk::CommandBuffer AllocatePrimary(
            vk::CommandPool command_pool,
            vk::Device      device);

        void Begin(vk::CommandBufferUsageFlags flags, vk::CommandBuffer buffer);
        void BeginOneTimeSubmit(vk::CommandBuffer buffer);

        void End(vk::CommandBuffer buffer);

        vk::CommandBufferSubmitInfo GetSubmitInfo(vk::CommandBuffer buffer);

        void CopyImageToImage(
            vk::Image           src_image,
            vk::Image           dst_image,
            const vk::Extent3D &src_extent,
            const vk::Extent3D &dst_extent,
            vk::CommandBuffer   command_buffer);

        void CopyBufferToBuffer(
            vk::Buffer        src_buffer,
            vk::Buffer        dst_buffer,
            uint64_t          src_offset,
            uint64_t          dst_offset,
            uint64_t          size,
            vk::CommandBuffer command_buffer);

        class BarrierMerger {
           public:
            BarrierMerger()  = default;
            ~BarrierMerger() = default;

            void clear();

            void transition_image_layout(
                vk::Image                  image,
                vk::ImageLayout            old_layout,
                vk::ImageLayout            new_layout,
                vk::PipelineStageFlags2 src_stage,
                vk::AccessFlags2        src_access,
                vk::PipelineStageFlags2 dst_stage,
                vk::AccessFlags2        dst_access);

            void put_buffer_barrier(
                vk::Buffer                 buffer,
                uint64_t                   offset,
                uint64_t                   size,
                vk::PipelineStageFlags2 src_stage,
                vk::AccessFlags2        src_access,
                vk::PipelineStageFlags2 dst_stage,
                vk::AccessFlags2        dst_access);

            void flushBarriers(vk::CommandBuffer command_buffer);

           private:
            std::vector<vk::ImageMemoryBarrier2>  m_ImageBarriers;
            std::vector<vk::BufferMemoryBarrier2> m_BufferBarriers;
        };
    }  // namespace CommandBuffer
}  // namespace Ignis::Vulkan