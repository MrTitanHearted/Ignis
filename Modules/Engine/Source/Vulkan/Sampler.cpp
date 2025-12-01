#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DestroySampler(const vk::Sampler sampler) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_Device.destroySampler(sampler);
    }

    vk::Sampler Vulkan::CreateSampler(const vk::SamplerCreateInfo &create_info) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        auto [result, sampler] = s_pInstance->m_Device.createSampler(create_info);
        DIGNIS_VK_CHECK(result);
        return sampler;
    }
}  // namespace Ignis