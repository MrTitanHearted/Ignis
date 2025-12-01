#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DestroyFence(const vk::Fence fence) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        s_pInstance->m_Device.destroyFence(fence);
    }

    void Vulkan::DestroySemaphore(const vk::Semaphore semaphore) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        s_pInstance->m_Device.destroySemaphore(semaphore);
    }

    vk::Fence Vulkan::CreateFence(const vk::FenceCreateFlags flags) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized");
        const vk::FenceCreateInfo create_info{flags};
        auto [result, fence] = s_pInstance->m_Device.createFence(create_info);
        DIGNIS_VK_CHECK(result);
        return fence;
    }

    vk::Semaphore Vulkan::CreateSemaphore() {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        constexpr vk::SemaphoreCreateInfo create_info{};
        auto [result, semaphore] = s_pInstance->m_Device.createSemaphore(create_info);
        DIGNIS_VK_CHECK(result);
        return semaphore;
    }

    vk::Semaphore Vulkan::CreateTimelineSemaphore(const uint64_t initial_value) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::SemaphoreTypeCreateInfo type_create_info{};
        type_create_info
            .setInitialValue(initial_value)
            .setSemaphoreType(vk::SemaphoreType::eTimeline);
        vk::SemaphoreCreateInfo create_info{};
        create_info.setPNext(&type_create_info);
        auto [result, semaphore] = s_pInstance->m_Device.createSemaphore(create_info);
        DIGNIS_VK_CHECK(result);
        return semaphore;
    }

    void Vulkan::ResetFences(const vk::ArrayProxy<vk::Fence> &fences) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        DIGNIS_VK_CHECK(s_pInstance->m_Device.resetFences(fences));
    }

    void Vulkan::WaitForAllFences(const vk::ArrayProxy<vk::Fence> &fences) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        DIGNIS_VK_CHECK(s_pInstance->m_Device.waitForFences(fences, vk::True, UINT32_MAX));
    }

    vk::SemaphoreSubmitInfo Vulkan::GetSemaphoreSubmitInfo(
        const vk::PipelineStageFlags2 stages,
        const vk::Semaphore           semaphore) {
        return vk::SemaphoreSubmitInfo{}
            .setStageMask(stages)
            .setSemaphore(semaphore);
    }

    vk::SemaphoreSubmitInfo Vulkan::GetTimelineSemaphoreSubmitInfo(
        const uint64_t                wait_value,
        const vk::PipelineStageFlags2 stages,
        const vk::Semaphore           semaphore) {
        return vk::SemaphoreSubmitInfo{}
            .setValue(wait_value)
            .setStageMask(stages)
            .setSemaphore(semaphore);
    }

    vk::SemaphoreWaitInfo Vulkan::GetTimelineSemaphoreWaitInfo(
        const vk::ArrayProxyNoTemporaries<vk::Semaphore> &semaphores,
        const vk::ArrayProxyNoTemporaries<uint64_t>      &wait_values) {
        return vk::SemaphoreWaitInfo{}
            .setSemaphores(semaphores)
            .setValues(wait_values);
    }
}  // namespace Ignis