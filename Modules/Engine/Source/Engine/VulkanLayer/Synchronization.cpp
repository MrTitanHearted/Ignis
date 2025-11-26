#include <Ignis/Engine/VulkanLayer/Synchronization.hpp>

namespace Ignis::Vulkan {
    namespace Fence {
        vk::Fence Create(const vk::Device device) {
            auto [result, fence] = device.createFence(vk::FenceCreateInfo{});
            DIGNIS_VK_CHECK(result);
            return fence;
        }

        vk::Fence CreateSignaled(const vk::Device device) {
            vk::FenceCreateInfo vk_fence_create_info{};
            vk_fence_create_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
            auto [result, fence] = device.createFence(vk_fence_create_info);
            DIGNIS_VK_CHECK(result);
            return fence;
        }
    }  // namespace Fence

    namespace Semaphore {
        vk::Semaphore Create(const vk::Device device) {
            auto [result, semaphore] = device.createSemaphore(vk::SemaphoreCreateInfo{});
            DIGNIS_VK_CHECK(result);
            return semaphore;
        }

        vk::Semaphore CreateTimeline(const vk::Device device) {
            vk::SemaphoreTypeCreateInfo vk_semaphore_type_create_info{};
            vk_semaphore_type_create_info
                .setInitialValue(0)
                .setSemaphoreType(vk::SemaphoreType::eTimeline);
            vk::SemaphoreCreateInfo vk_semaphore_create_info{};
            vk_semaphore_create_info.setPNext(&vk_semaphore_type_create_info);
            auto [result, semaphore] = device.createSemaphore(vk_semaphore_create_info);
            DIGNIS_VK_CHECK(result);
            return semaphore;
        }

        vk::Semaphore CreateTimeline(const uint64_t initial_value, const vk::Device device) {
            vk::SemaphoreTypeCreateInfo vk_semaphore_type_create_info{};
            vk_semaphore_type_create_info
                .setInitialValue(initial_value)
                .setSemaphoreType(vk::SemaphoreType::eTimeline);
            vk::SemaphoreCreateInfo vk_semaphore_create_info{};
            vk_semaphore_create_info.setPNext(&vk_semaphore_type_create_info);
            auto [result, semaphore] = device.createSemaphore(vk_semaphore_create_info);
            DIGNIS_VK_CHECK(result);
            return semaphore;
        }

        vk::SemaphoreSubmitInfo GetSubmitInfo(
            const vk::PipelineStageFlags2 stage_flags,
            const vk::Semaphore           semaphore) {
            return vk::SemaphoreSubmitInfo{}
                .setStageMask(stage_flags)
                .setSemaphore(semaphore);
        }

        vk::SemaphoreSubmitInfo GetSubmitInfo(
            const uint64_t                value,
            const vk::PipelineStageFlags2 stage_flags,
            const vk::Semaphore           semaphore) {
            return vk::SemaphoreSubmitInfo{}
                .setValue(value)
                .setStageMask(stage_flags)
                .setSemaphore(semaphore);
        }

        vk::SemaphoreWaitInfo GetWaitInfo(
            const vk::ArrayProxyNoTemporaries<vk::Semaphore> &semaphores,
            const vk::ArrayProxyNoTemporaries<uint64_t>      &values) {
            return vk::SemaphoreWaitInfo{}
                .setSemaphores(semaphores)
                .setValues(values);
        }
    }  // namespace Semaphore
}  // namespace Ignis::Vulkan