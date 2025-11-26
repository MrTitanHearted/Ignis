#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan::Queue {
    vk::Result Present(
        const vk::ArrayProxy<vk::Semaphore> &wait_semaphores,
        uint32_t                             image_index,
        vk::SwapchainKHR                     swapchain,
        vk::Queue                            queue);

    vk::Result Present(
        uint32_t         image_index,
        vk::SwapchainKHR swapchain,
        vk::Queue        queue);

    void Submit(
        const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,
        vk::Fence                                          fence,
        vk::Queue                                          queue);

    void Submit(
        const vk::ArrayProxy<vk::CommandBufferSubmitInfo> &command_buffer_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &wait_semaphore_infos,
        const vk::ArrayProxy<vk::SemaphoreSubmitInfo>     &signal_semaphore_infos,
        vk::Queue                                          queue);

}  // namespace Ignis::Vulkan::Queue