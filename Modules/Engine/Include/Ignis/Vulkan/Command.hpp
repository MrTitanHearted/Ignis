#pragma once

#include <Ignis/Vulkan/Types.hpp>

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
    }  // namespace CommandBuffer
}  // namespace Ignis::Vulkan