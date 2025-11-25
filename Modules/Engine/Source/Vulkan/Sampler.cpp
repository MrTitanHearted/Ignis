#include <Ignis/Vulkan/Sampler.hpp>

namespace Ignis::Vulkan::Sampler {
    vk::Sampler Create(
        const vk::SamplerCreateInfo &vk_sampler_create_info,
        const vk::Device             device) {
        auto [result, vk_sampler] = device.createSampler(vk_sampler_create_info);
        DIGNIS_VK_CHECK(result);
        return vk_sampler;
    }
}  // namespace Ignis::Vulkan::Sampler