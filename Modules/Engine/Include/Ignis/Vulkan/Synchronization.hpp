#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan {
    namespace Fence {
        vk::Fence Create(vk::Device device);
        vk::Fence CreateSignaled(vk::Device device);
    }  // namespace Fence

    namespace Semaphore {
        vk::Semaphore Create(vk::Device device);
        vk::Semaphore CreateTimeline(vk::Device device);
        vk::Semaphore CreateTimeline(uint64_t initial_value, vk::Device device);

        vk::SemaphoreSubmitInfo GetSubmitInfo(
            vk::PipelineStageFlags2 stage_flags,
            vk::Semaphore           semaphore);

        vk::SemaphoreSubmitInfo GetSubmitInfo(
            uint64_t                value,
            vk::PipelineStageFlags2 stage_flags,
            vk::Semaphore           semaphore);

        vk::SemaphoreWaitInfo GetWaitInfo(
            const vk::ArrayProxyNoTemporaries<vk::Semaphore> &semaphores,
            const vk::ArrayProxyNoTemporaries<uint64_t>      &values);
    }  // namespace Semaphore
}  // namespace Ignis::Vulkan