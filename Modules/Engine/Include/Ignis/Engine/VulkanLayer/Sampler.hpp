#pragma once

#include <Ignis/Engine/VulkanLayer/Types.hpp>

namespace Ignis::Vulkan::Sampler {
    vk::Sampler Create(const vk::SamplerCreateInfo &vk_sampler_create_info, vk::Device device);
}