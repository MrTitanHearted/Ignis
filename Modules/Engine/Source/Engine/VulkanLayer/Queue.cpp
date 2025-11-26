#include <Ignis/Engine/VulkanLayer/Queue.hpp>

namespace Ignis::Vulkan::Queue {

    vk::Result Present(
        const vk::ArrayProxy<vk::Semaphore> &wait_semaphores,
        const uint32_t                       image_index,
        const vk::SwapchainKHR               swapchain,
        const vk::Queue                      queue) {
        vk::PresentInfoKHR vk_present_info{};
        vk_present_info
            .setWaitSemaphores(wait_semaphores)
            .setImageIndices(image_index)
            .setSwapchains(swapchain);
        return static_cast<vk::Result>(vkQueuePresentKHR(
            queue,
            &static_cast<const VkPresentInfoKHR &>(vk_present_info)));
    }

    vk::Result Present(
        const uint32_t         image_index,
        const vk::SwapchainKHR swapchain,
        const vk::Queue        queue) {
        return Present({}, image_index, swapchain, queue);
    }

    void Submit(
        const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,
        const vk::Fence                                    fence,
        const vk::Queue                                    queue) {
        vk::SubmitInfo2 vk_submit_info{};
        vk_submit_info
            .setCommandBufferInfos(command_buffer_infos)
            .setWaitSemaphoreInfos(wait_semaphore_infos)
            .setSignalSemaphoreInfos(signal_semaphore_infos);
        DIGNIS_VK_CHECK(queue.submit2(vk_submit_info, fence));
    }

    void Submit(
        const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,
        const vk::Queue                                    queue) {
        vk::SubmitInfo2 vk_submit_info{};
        vk_submit_info
            .setCommandBufferInfos(command_buffer_infos)
            .setWaitSemaphoreInfos(wait_semaphore_infos)
            .setSignalSemaphoreInfos(signal_semaphore_infos);
        DIGNIS_VK_CHECK(queue.submit2(vk_submit_info));
    }
}  // namespace Ignis::Vulkan::Queue