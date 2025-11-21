#pragma once

#include <Ignis/Vulkan/Types.hpp>

namespace Ignis::Vulkan {
    namespace Image {
        vk::ImageAspectFlags GetAspectMask(vk::ImageLayout layout);

        void TransitionLayout(
            vk::Image         image,
            vk::ImageLayout   old_layout,
            vk::ImageLayout   new_layout,
            vk::CommandBuffer command_buffer);

        void TransitionLayout(
            vk::Image         image,
            vk::ImageLayout   new_layout,
            vk::CommandBuffer command_buffer);
    }  // namespace Image

    namespace ImageView {}
}  // namespace Ignis::Vulkan