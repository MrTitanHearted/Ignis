#include <Ignis/Vulkan.hpp>

namespace Ignis {
    vk::Result Vulkan::Present(
        const vk::ArrayProxy<vk::Semaphore> &wait_semaphores,
        const uint32_t                       image_index) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::PresentInfoKHR present_info{};
        present_info
            .setWaitSemaphores(wait_semaphores)
            .setImageIndices(image_index)
            .setSwapchains(s_pInstance->m_Swapchain);
        return static_cast<vk::Result>(vkQueuePresentKHR(
            s_pInstance->m_PresentQueue,
            &static_cast<const VkPresentInfoKHR &>(present_info)));
    }

    void Vulkan::Submit(
        const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,

        const vk::Fence fence,
        const vk::Queue queue) {
        vk::SubmitInfo2 submit_info{};
        submit_info
            .setCommandBufferInfos(command_buffer_infos)
            .setWaitSemaphoreInfos(wait_semaphore_infos)
            .setSignalSemaphoreInfos(signal_semaphore_infos);
        DIGNIS_VK_CHECK(queue.submit2(submit_info, fence));
    }
}  // namespace Ignis