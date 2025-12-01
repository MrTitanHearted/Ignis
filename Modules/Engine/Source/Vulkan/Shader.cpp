#include <Ignis/Vulkan.hpp>

namespace Ignis {
    void Vulkan::DestroyShaderModule(const vk::ShaderModule shader_module) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        s_pInstance->m_Device.destroyShaderModule(shader_module);
    }

    vk::ShaderModule Vulkan::CreateShaderModuleFromSPV(const std::span<uint32_t> code) {
        DIGNIS_ASSERT(nullptr != s_pInstance, "Ignis::Vulkan is not initialized.");
        vk::ShaderModuleCreateInfo create_info{};
        create_info.setCode(code);

        auto [result, module] = s_pInstance->m_Device.createShaderModule(create_info);
        DIGNIS_VK_CHECK(result);
        return module;
    }
}  // namespace Ignis